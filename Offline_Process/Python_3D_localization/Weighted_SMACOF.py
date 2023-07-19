from numbers import Integral, Real

from sklearn.utils import check_random_state, check_array, check_symmetric

from sklearn.base import BaseEstimator
from sklearn.metrics import euclidean_distances
from sklearn.isotonic import IsotonicRegression
import numpy as np

import warnings


def _smacof_single(
    dissimilarities,
    weight_matrix = None,
    metric=True,
    n_components=2,
    init=None,
    max_iter=300,
    verbose=0,
    eps=1e-3,
    random_state=None,
    normalized_stress=False,
):
    """Computes multidimensional scaling using SMACOF algorithm.
    Parameters
    ----------
    dissimilarities : ndarray of shape (n_samples, n_samples)
        Pairwise dissimilarities between the points. Must be symmetric.
    metric : bool, default=True
        Compute metric or nonmetric SMACOF algorithm.
        When ``False`` (i.e. non-metric MDS), dissimilarities with 0 are considered as
        missing values.
    n_components : int, default=2
        Number of dimensions in which to immerse the dissimilarities. If an
        ``init`` array is provided, this option is overridden and the shape of
        ``init`` is used to determine the dimensionality of the embedding
        space.
    init : ndarray of shape (n_samples, n_components), default=None
        Starting configuration of the embedding to initialize the algorithm. By
        default, the algorithm is initialized with a randomly chosen array.
    max_iter : int, default=300
        Maximum number of iterations of the SMACOF algorithm for a single run.
    verbose : int, default=0
        Level of verbosity.
    eps : float, default=1e-3
        Relative tolerance with respect to stress at which to declare
        convergence. The value of `eps` should be tuned separately depending
        on whether or not `normalized_stress` is being used.
    random_state : int, RandomState instance or None, default=None
        Determines the random number generator used to initialize the centers.
        Pass an int for reproducible results across multiple function calls.
        See :term:`Glossary <random_state>`.
    normalized_stress : bool, default=False
        Whether use and return normed stress value (Stress-1) instead of raw
        stress calculated by default. Only supported in non-metric MDS. The
        caller must ensure that if `normalized_stress=True` then `metric=False`
        .. versionadded:: 1.2
    Returns
    -------
    X : ndarray of shape (n_samples, n_components)
        Coordinates of the points in a ``n_components``-space.
    stress : float
        The final value of the stress (sum of squared distance of the
        disparities and the distances for all constrained points).
        If `normalized_stress=True`, and `metric=False` returns Stress-1.
        A value of 0 indicates "perfect" fit, 0.025 excellent, 0.05 good,
        0.1 fair, and 0.2 poor [1]_.
    n_iter : int
        The number of iterations corresponding to the best stress.
    References
    ----------
    .. [1] "Nonmetric multidimensional scaling: a numerical method" Kruskal, J.
           Psychometrika, 29 (1964)
    .. [2] "Multidimensional scaling by optimizing goodness of fit to a nonmetric
           hypothesis" Kruskal, J. Psychometrika, 29, (1964)
    .. [3] "Modern Multidimensional Scaling - Theory and Applications" Borg, I.;
           Groenen P. Springer Series in Statistics (1997)
    """
    dissimilarities = check_symmetric(dissimilarities, raise_exception=True)

    n_samples = dissimilarities.shape[0]
    random_state = check_random_state(random_state)

    sim_flat = ((1 - np.tri(n_samples)) * dissimilarities).ravel()
    sim_flat_w = sim_flat[sim_flat != 0]
    if init is None:
        # Randomly choose initial configuration
        X = random_state.uniform(size=n_samples * n_components)
        X = X.reshape((n_samples, n_components))
    else:
        # overrides the parameter p
        n_components = init.shape[1]
        if n_samples != init.shape[0]:
            raise ValueError(
                "init matrix should be of shape (%d, %d)" % (n_samples, n_components)
            )
        X = init

    old_stress = None
    ir = IsotonicRegression()

    ### set up the weight matrix
    if weight_matrix is None:
        weights = np.ones((n_samples, n_samples))
    else:
        weights = check_symmetric(weight_matrix, raise_exception=True)
    diag = np.arange(n_samples)
    weights[diag, diag] = 0
    # print("weights: ", weights)
    for it in range(max_iter):
        # Compute distance and monotonic regression
        dis = euclidean_distances(X)

        if metric:
            disparities = dissimilarities
        else:
            dis_flat = dis.ravel()
            # dissimilarities with 0 are considered as missing values
            dis_flat_w = dis_flat[sim_flat != 0]

            # Compute the disparities using a monotonic regression
            disparities_flat = ir.fit_transform(sim_flat_w, dis_flat_w)
            disparities = dis_flat.copy()
            disparities[sim_flat != 0] = disparities_flat
            disparities = disparities.reshape((n_samples, n_samples))
            disparities *= np.sqrt(
                (n_samples * (n_samples - 1) / 2) / (disparities**2).sum()
            )

        # Compute stress
        # print(weights.ravel())
        # print(dis.ravel())
        # print(disparities.ravel())
        stress = (weights.ravel()*(dis.ravel() - disparities.ravel()) ** 2).sum() / 2

        # print("stress:", stress)
        # raise KeyboardInterrupt
        if normalized_stress:
            stress = np.sqrt(stress / ((disparities.ravel() ** 2).sum() / 2))
        # Update X using the Guttman transform
        dis[dis == 0] = 1e-5
        ratio = weights*disparities / dis
        B = -ratio
        B[np.arange(len(B)), np.arange(len(B))] += ratio.sum(axis=1)
        
        V = -1*weights
        V[diag, diag] += weights.sum(axis=1)
        V_inv = np.linalg.pinv(V)
        # print("V: ", V, V_inv)
        X = V_inv.dot(np.dot(B, X))

        # X = 1.0 / n_samples * np.dot(B, X)

        dis = np.sqrt((X**2).sum(axis=1)).sum()

        # print("it: %d, stress %s" % (it, stress))
        if verbose >= 2:
            print("it: %d, stress %s" % (it, stress))
        if old_stress is not None:
            if (old_stress - stress / dis) < eps:
                if verbose:
                    print("breaking at iteration %d with stress %s" % (it, stress))
                break
        old_stress = stress / dis

    return X, stress, it + 1


def smacof(
    dissimilarities,
    *,
    weight_matrix = None,
    metric=True,
    n_components=2,
    init=None,
    n_init=8,
    n_jobs=None,
    max_iter=300,
    verbose=0,
    eps=1e-3,
    random_state=None,
    return_n_iter=False,
    normalized_stress="warn",
):
    """Compute multidimensional scaling using the SMACOF algorithm.
    The SMACOF (Scaling by MAjorizing a COmplicated Function) algorithm is a
    multidimensional scaling algorithm which minimizes an objective function
    (the *stress*) using a majorization technique. Stress majorization, also
    known as the Guttman Transform, guarantees a monotone convergence of
    stress, and is more powerful than traditional techniques such as gradient
    descent.
    The SMACOF algorithm for metric MDS can be summarized by the following
    steps:
    1. Set an initial start configuration, randomly or not.
    2. Compute the stress
    3. Compute the Guttman Transform
    4. Iterate 2 and 3 until convergence.
    The nonmetric algorithm adds a monotonic regression step before computing
    the stress.
    Parameters
    ----------
    dissimilarities : ndarray of shape (n_samples, n_samples)
        Pairwise dissimilarities between the points. Must be symmetric.
    metric : bool, default=True
        Compute metric or nonmetric SMACOF algorithm.
        When ``False`` (i.e. non-metric MDS), dissimilarities with 0 are considered as
        missing values.
    n_components : int, default=2
        Number of dimensions in which to immerse the dissimilarities. If an
        ``init`` array is provided, this option is overridden and the shape of
        ``init`` is used to determine the dimensionality of the embedding
        space.
    init : ndarray of shape (n_samples, n_components), default=None
        Starting configuration of the embedding to initialize the algorithm. By
        default, the algorithm is initialized with a randomly chosen array.
    n_init : int, default=8
        Number of times the SMACOF algorithm will be run with different
        initializations. The final results will be the best output of the runs,
        determined by the run with the smallest final stress. If ``init`` is
        provided, this option is overridden and a single run is performed.
    n_jobs : int, default=None
        The number of jobs to use for the computation. If multiple
        initializations are used (``n_init``), each run of the algorithm is
        computed in parallel.
        ``None`` means 1 unless in a :obj:`joblib.parallel_backend` context.
        ``-1`` means using all processors. See :term:`Glossary <n_jobs>`
        for more details.
    max_iter : int, default=300
        Maximum number of iterations of the SMACOF algorithm for a single run.
    verbose : int, default=0
        Level of verbosity.
    eps : float, default=1e-3
        Relative tolerance with respect to stress at which to declare
        convergence. The value of `eps` should be tuned separately depending
        on whether or not `normalized_stress` is being used.
    random_state : int, RandomState instance or None, default=None
        Determines the random number generator used to initialize the centers.
        Pass an int for reproducible results across multiple function calls.
        See :term:`Glossary <random_state>`.
    return_n_iter : bool, default=False
        Whether or not to return the number of iterations.
    normalized_stress : bool or "auto" default=False
        Whether use and return normed stress value (Stress-1) instead of raw
        stress calculated by default. Only supported in non-metric MDS.
        .. versionadded:: 1.2
    Returns
    -------
    X : ndarray of shape (n_samples, n_components)
        Coordinates of the points in a ``n_components``-space.
    stress : float
        The final value of the stress (sum of squared distance of the
        disparities and the distances for all constrained points).
        If `normalized_stress=True`, and `metric=False` returns Stress-1.
        A value of 0 indicates "perfect" fit, 0.025 excellent, 0.05 good,
        0.1 fair, and 0.2 poor [1]_.
    n_iter : int
        The number of iterations corresponding to the best stress. Returned
        only if ``return_n_iter`` is set to ``True``.
    References
    ----------
    .. [1] "Nonmetric multidimensional scaling: a numerical method" Kruskal, J.
           Psychometrika, 29 (1964)
    .. [2] "Multidimensional scaling by optimizing goodness of fit to a nonmetric
           hypothesis" Kruskal, J. Psychometrika, 29, (1964)
    .. [3] "Modern Multidimensional Scaling - Theory and Applications" Borg, I.;
           Groenen P. Springer Series in Statistics (1997)
    """

    dissimilarities = check_array(dissimilarities)
    random_state = check_random_state(random_state)

    # TODO(1.4): Remove
    if normalized_stress == "warn":
        # warnings.warn(
        #     "The default value of `normalized_stress` will change to `'auto'` in"
        #     " version 1.4. To suppress this warning, manually set the value of"
        #     " `normalized_stress`.",
        #     FutureWarning,
        # )
        normalized_stress = False

    if normalized_stress == "auto":
        normalized_stress = not metric

    if normalized_stress and metric:
        raise ValueError(
            "Normalized stress is not supported for metric MDS. Either set"
            " `normalized_stress=False` or use `metric=False`."
        )
    if hasattr(init, "__array__"):
        init = np.asarray(init).copy()
        if not n_init == 1:
            warnings.warn(
                "Explicit initial positions passed: "
                "performing only one init of the MDS instead of %d" % n_init
            )
            n_init = 1

    best_pos, best_stress = None, None

    for it in range(n_init):
        pos, stress, n_iter_ = _smacof_single(
            dissimilarities,
            weight_matrix = weight_matrix,
            metric=metric,
            n_components=n_components,
            init=init,
            max_iter=max_iter,
            verbose=verbose,
            eps=eps,
            random_state=random_state,
            normalized_stress=normalized_stress,
        )
        if best_stress is None or stress < best_stress:
            best_stress = stress
            best_pos = pos.copy()
            best_iter = n_iter_
   
    if return_n_iter:
        return best_pos, best_stress, best_iter
    else:
        return best_pos, best_stress