import numpy as np 
import matplotlib.pyplot as plt
import os
import seaborn as sb
import matplotlib
# from sklearn.manifold import smacof
from Weighted_SMACOF import smacof
colors = {'blue':'#1d7fb8', 'gold': '#e1971b', 'green': '#059567', 'brown': '#d9775a', 'purple':'#ca18de', 'red': '#ac0000' }
import networkx as nx

# color_lists = ["tab:blue", "tab:orange", "tab:green", "tab:red", "tab:purple", "tab:brown"] 

color_lists = [colors['blue'], colors['red'], colors['green'], colors['brown'], colors['purple'], colors['gold']]
matplotlib.rc('font', serif='Helvetica') 
legend_size = 15
label_size = 15
tick_size = 14
np.random.seed(0)

def check_connectivity(dis_matrix):
    G = nx.Graph()
    edges = []
    N = dis_matrix.shape[0]
    for i in range(0, N):
        for j in range(i+1, N):
            if dis_matrix[i][j] > 0:
                edges.append((i, j))
    G.add_edges_from(edges)
    min_cut = nx.minimum_node_cut(G)
    return len(min_cut)

def mds(d, dimensions = 2):
    """
    Multidimensional Scaling - Given a matrix of interpoint distances,
    find a set of low dimensional points that have similar interpoint
    distances.
    """

    (n,n) = d.shape
    E = (-0.5 * d**2)

    # Use mat to get column and row means to act as column and row means.
    Er = np.mat(np.mean(E,1))
    Es = np.mat(np.mean(E,0))

    # From Principles of Multivariate Analysis: A User's Perspective (page 107).
    F = np.array(E - np.transpose(Er) - Es + np.mean(E))

    [U, S, V] = np.linalg.svd(F)

    Y = U * np.sqrt(S)

    return (Y[:,0:dimensions], S)

def calculate_dis_matrix(pos):
    num_users = pos.shape[0]
    dis_gt = np.zeros((num_users, num_users))
    for i in range(num_users):
        for j in range(i + 1, num_users):
            tmp_d = np.linalg.norm(pos[i, :] - pos[j, :] )
            dis_gt[i, j] = tmp_d
            dis_gt[j, i] = tmp_d

    return dis_gt

def plot_cdf_list(data:list, labels: list):
    fig, ax = plt.subplots()


    x_50 = []
    x_90 = []
    mean_list = []
    for i in range(len(data)):
        sb.ecdfplot(data[i], ax=ax, label=labels[i], linewidth=2, color = color_lists[i])
        x_50.append(np.percentile(data[i], 50))
        x_90.append(np.percentile(data[i], 95))
        mean_list.append(np.mean(data[i]))
    print("mean_list: ", mean_list)
    print("x_50", x_50)
    print("x_90", x_90)
    # ax.grid()
    ax.legend(fontsize = legend_size, frameon=False)
    
    large_50 = np.amax(x_50)
    large_90 = np.amax(x_90)

    ax.hlines(0.5, -0, large_50, 'k', linestyles = 'dashed')
    ax.hlines(0.95, -0, large_90, 'k', linestyles = 'dashed')
    
    for i in range(len(data)):
        ax.vlines(x_50[i], 0, 0.5, color_lists[i], linestyles = 'dashed')
        ax.vlines(x_90[i], 0, 0.95, color_lists[i], linestyles = 'dashed')

    ax.set_xlabel('2D Localization Error (cm)', fontsize=label_size)
    ax.set_ylabel('CDF', fontsize=label_size)
    ax.tick_params(axis='both',labelsize=tick_size) 
    ax.set_yticks([0.0, 0.2, 0.4, 0.5, 0.6, 0.8, 0.95, 1])

def plot_cdf(data:np.ndarray, xlabel: str):
    fig, ax = plt.subplots()

    sb.ecdfplot(data, ax=ax, linewidth=2)
    
    ax.grid()
    # ax.legend()
    
    ax.set_xlabel(xlabel, fontsize=14)
    ax.set_ylabel('CDF', fontsize=14)


err_1D = 0.8
outlier_err = [3, 20]
out_lier_ratio = 0
MAX_RANGE = 30
drop_rate = 0
MAX_HEIGHT = 10
height_err = 0.4

pointing_err = 0/180*np.pi

def smacof_outlier_detect(dis_measure, weights = None, outlier = False):
    N = dis_measure.shape[0]
    if weights is None:
        weight_measure = np.ones((N, N)) 
    else:
        weight_measure = weights
    
    pos_pred, score = smacof(dissimilarities=dis_measure, weight_matrix=weight_measure, n_components=2, metric = True,  max_iter = 1000, eps = 1e-4, random_state=0, n_init=8)
    best_score = max([score * (N*(N-1)  - 1)/ (N*(N - 1)) , 0])
    best_pred = pos_pred   
    best_score_init = best_score
    print("smacof score: ", best_score)

    if outlier and best_score > 2:
        for i in range(N):
            for j in range(i+1, N):
                weight0 = np.copy(weight_measure)
                weight0[i, j] = 0
                weight0[j, i] = 0
                pos_pred0, score0 = smacof(dissimilarities=dis_measure, weight_matrix=weight0, n_components=2, metric = True,  max_iter = 1000, eps = 1e-4, random_state=0, n_init=8)
                print(i, j, score0, best_score)
                if score0 < best_score_init*0.2 and score0 < best_score:
                    best_score = score0
                    best_pred = pos_pred0
        
        print("best smacof score: ", best_score)
        return best_pred
    else:
        return best_pred


def check_closet(pos, pos_list):
    for p1 in pos_list:
        if np.linalg.norm( np.array(pos[:2]) - np.array(p1[:2]) ) < 3:
            return False
    return True



def generate_diver_pos(exist_lists):
    while True:
        x = np.random.uniform(low = -MAX_RANGE, high = MAX_RANGE)
        if abs(x) > 3:
            break
    while True:
        y = np.random.uniform(low = -MAX_RANGE, high = MAX_RANGE)
        if abs(y) > 3:
            break
    z = np.random.uniform(low = 0, high = MAX_HEIGHT)
    pos = [x,y,z]

    if check_closet(pos, exist_lists):
        return pos 
    else:
        return  generate_diver_pos(exist_lists)
    



def drop(dis_matrix, weights, drop_num):
    N = dis_matrix.shape[0]
    indexes = [i for i in range(N)]
    drop_choice = []
    for i in range(N):
        for j in range(i + 1, N):
            drop_choice.append([i, j])

    for t in range(drop_num):
        iter_num = 0
        while True:
            iter_num += 1
            if len(drop_choice) <= 0:
                print("Warning!!!!!! no more link can be dropped")
                print("already drop ", t)
                break
            indexes = [i for i in range(len(drop_choice))]
            idx = np.random.choice(indexes)
            dis_matrix0 = np.copy(dis_matrix)
            dis_matrix0[drop_choice[idx][0]][drop_choice[idx][1]] = 0
            dis_matrix0[drop_choice[idx][1]][drop_choice[idx][0]] = 0
            conn = check_connectivity(dis_matrix0)

            drop_idx = drop_choice.pop(idx)

            if conn >= 3: ## success removal
                print("drop: ", drop_idx)
                dis_matrix[drop_idx[0]][drop_idx[1]] = 0
                dis_matrix[drop_idx[1]][drop_idx[0]] = 0
                weights[drop_idx[1]][drop_idx[0]] = 0
                weights[drop_idx[0]][drop_idx[1]] = 0
                break
            else: ## try again
                continue
    
    return dis_matrix, weights
            


def simulate_one_sample(idx):
    num_users = 5
    user1_dis = np.random.uniform(low = 4, high = 9)

    h_leader = np.random.uniform(low = 0, high = MAX_HEIGHT)
    h_user0 = np.random.uniform(low = 0, high = MAX_HEIGHT)
    pos_gt = [
        [0, 0, h_leader],
        [user1_dis, 0, h_user0]
    ]

    for i in range(num_users - 1):
        # while True:
        #     x = np.random.uniform(low = -MAX_RANGE, high = MAX_RANGE)
        #     if abs(x) > 5:
        #         break
        # while True:
        #     y = np.random.uniform(low = -MAX_RANGE, high = MAX_RANGE)
        #     if abs(y) > 5:
        #         break
        # z = np.random.uniform(low = 0, high = MAX_HEIGHT)
        # pos_gt.append([x, y, z])
        pos = generate_diver_pos(pos_gt)
        pos_gt.append(pos)

    pos_gt = np.array(pos_gt) #np.concatenate([pos_gt, pos_users], axis = 0)
    
    N = pos_gt.shape[0]
    dis_gt_2d = np.zeros((N, N))
    dis_gt = np.zeros((N, N))
    dis_measure = np.zeros((N, N))
    height_measure = np.zeros((N, ))
    weight_measure = np.ones((N, N))
    add_err = []

    err_dis = [[], [], []]

    for i in range(N):
        height_measure[i] = pos_gt[i, 2] + np.random.uniform(low = -height_err, high = height_err)

    for i in range(N):
        for j in range(i + 1, N):
            tmp_d = np.linalg.norm(pos_gt[i, :] - pos_gt[j, :] )  
            tmp_d_2d = np.linalg.norm(pos_gt[i, :2] - pos_gt[j, :2] )  
            dis_gt[i, j] = tmp_d
            dis_gt[j, i] = tmp_d
            dis_gt_2d[i, j] = tmp_d_2d
            dis_gt_2d[j, i] = tmp_d_2d
            ###  add noise 
           
            err =  np.random.uniform(low = -err_1D, high = err_1D)
            add_err.append(err)

            dis_measure[i, j]= tmp_d + err 
            dis_measure[j, i]= tmp_d + err 
    # print("add errs: ", add_err)
    # print(dis_gt_2d)
    ### project to 2D plane 
    dis_measure2 = np.zeros_like(dis_measure)
    for a in range(0, N):
        for b in range(a+1, N):
            if idx == 88:
                print(dis_measure[a, b])
                print(height_measure[a] - height_measure[b])
            if dis_measure[a, b]**2 - (height_measure[a] - height_measure[b] )**2 < 0 :
                print(dis_measure[a, b], height_measure[a] - height_measure[b] )
                print("Warning!!!!! drop it")
                return [], [], [], err_dis
            calib_dis = np.sqrt(dis_measure[a, b]**2 - (height_measure[a] - height_measure[b] )**2 )
            dis_measure2[a, b] = calib_dis
            dis_measure2[b, a] = calib_dis


    ### msd
    # pos_pred, _ = mds(dis_measure)
    ### add outlier:
    p_outlire = np.random.uniform(low = 0, high = 1)
    if p_outlire < out_lier_ratio:
        indexes = [i for i in range(N)]
        drop_pair = np.random.choice(indexes, size=(2, ), replace=False)
        err = np.random.uniform(low = outlier_err[0], high = outlier_err[1])
        dis_measure2[drop_pair[0], drop_pair[1]] += err
        dis_measure2[drop_pair[1], drop_pair[0]] += err
        print("outlire happens in ", i, j )

    ####  drop the link 
    dis_measure2 , weight_measure = drop(dis_measure2 , weight_measure, 3)
    # print(dis_measure2 , weight_measure)
    # p_drop = np.random.uniform(low = 0, high = 1)
    # if p_drop < drop_rate:
    #     indexes = [i for i in range(N)]
    #     drop_pair = np.random.choice(indexes, size=(2, ), replace=False)
    #     print("Drop happend in pair: ", drop_pair)
    #     dis_measure[drop_pair[0], drop_pair[1]] = 0 
    #     dis_measure[drop_pair[1], drop_pair[0]] = 0
    #     weight_measure[drop_pair[0], drop_pair[1]] = 0
    #     weight_measure[drop_pair[1], drop_pair[0]] = 0

    ### smaco
    # print(dis_measure - dis_gt )
    # pos_pred, score = smacof(dissimilarities=dis_measure, weight_matrix=weight_measure, n_components=2, metric = True,  max_iter = 1000, eps = 1e-4, random_state=0, n_init=8)
    
    pos_pred = smacof_outlier_detect(dis_measure = dis_measure2, weights=weight_measure, outlier= False)


    # if score > 100:
    #     for i in range(N):
    #         for j in range(N):
    #             weight0 = np.copy(weight_measure)
    #             weight0[i, j] = 0.1
    #             weight0[j, i] = 0.1
    #             pos_pred0, score0 = smacof(dissimilarities=dis_measure, weight_matrix=weight0, n_components=2, metric = True,  max_iter = 1000, eps = 1e-4, random_state=0, n_init=8)
    #             print(i, j, score0, best_score)
    #             if score0 < best_score:
    #                 best_score = score0
    #                 best_pred = pos_pred0

    pos_pred = pos_pred - pos_pred[0, :]

    angle = -np.arctan2(pos_pred[1, 1] , pos_pred[1, 0])
    angle = angle +  np.random.uniform(low = -pointing_err, high = +pointing_err)
    R = np.array([
        [np.cos(angle), np.sin(-angle) ],
        [np.sin(angle), np.cos(-angle) ]
    ])
    pos2 = (R.dot(pos_pred.T)).T


    pos2_flip = np.copy(pos2)
    pos2_flip[:, 1] = pos2_flip[:, 1] * (-1)
    err1 = np.mean(np.linalg.norm(pos2 - pos_gt[:, :2], axis = 1))
    err2 = np.mean(np.linalg.norm(pos2_flip - pos_gt[:, :2], axis = 1))

    if err2 < err1:
        pos2 = pos2_flip

    pred_dis = calculate_dis_matrix(pos2)

    range_errs = []
    AOA_err = []
    err_2d = []

    for a in range(0, N):
        for b in range(a+1,  N):
            if(dis_measure[a, b] > 0):
                err_1d = np.abs(dis_gt[a, b] - pred_dis[a, b])
                range_errs.append(err_1d)

    for a in range(0, N):
        if a == 0 or a == 1:
            continue
        AOA_gt = np.arctan2(pos_gt[a, 1], pos_gt[a, 0]) 
        AOA_pred  = np.arctan2(pos2[a, 1], pos2[a, 0]) 
        errs = [np.abs(np.rad2deg(AOA_gt - AOA_pred)), np.abs(np.rad2deg(AOA_gt - AOA_pred) + 180),np.abs(np.rad2deg(AOA_gt - AOA_pred) + 180), np.abs(np.rad2deg(AOA_gt - AOA_pred) - 360), np.abs(np.rad2deg(AOA_gt - AOA_pred) + 360)]
        AOA_err.append(min(errs))
    
    for a in range(0, N):
        if a == 0:
            continue
        err = np.linalg.norm(pos_gt[a, :2] - pos2[a, :2])

        dis_leader = (dis_gt_2d[a, 0 ] +dis_gt_2d[a, 1])/2 
        if dis_leader < 10:
            err_dis[0].append(err)
        elif dis_leader >= 10 and dis_leader < 20:
            err_dis[1].append(err)
        else:
            err_dis[2].append(err)
        err_2d.append(err)
    # print(err_2d)
    # print(range_errs)
    if np.amax(err_2d) > 10 or np.amax(AOA_err) > 60:
        # print(pred_dis)
        # print(dis_gt)
        print("******* MAX_err: ", np.amax(range_errs), np.amax(AOA_err), np.amax(err_2d))
        # # print("add errs: ", add_err)
    # plt.figure()
    
    # for i in range(pos_gt.shape[0]):
    #     plt.scatter(pos2[i, 0], pos2[i, 1])
    #     plt.scatter(pos_gt[i, 0], pos_gt[i, 1], color = "black",marker = 'x')
    #     plt.text(pos2[i, 0]+0.2, pos2[i, 1]+0.2, str(i))
    # plt.xlim([-32, 35])
    # plt.ylim([-32, 35])
    # plt.show()
    
    return range_errs, AOA_err, err_2d, err_dis

if __name__ == "__main__":
    # m = np.array([
    #     [0,1,1,1,1],
    #     [1,0,1,0,1],
    #     [1,1,0,1,0],
    #     [1,0,1,0,1],
    #     [1,1,0,1,0]
    # ])
    # c =  check_connectivity(m)

    list_1d =  []
    list_AOA=  []
    list_2d = []
    list_dis = [[], [], []]
    for i in range(0, 200):
        print("*"*10, i)
        range_errs, AOA_err, err_2d, err_dis = simulate_one_sample(i)
        list_1d.extend(range_errs)
        list_AOA.extend(AOA_err)
        list_2d.extend(err_2d)
        list_dis[0].extend(err_dis[0])
        list_dis[1].extend(err_dis[1])
        list_dis[2].extend(err_dis[2])

    plot_cdf(np.array(list_1d), '1d err')
    plot_cdf(np.array(list_AOA), 'AOA err')
    plot_cdf_list([np.array(list_2d)], ['2d err'])
    print("Mean : ", np.mean(np.array(list_2d)),"STD : ", np.std(np.array(list_2d)) )
    plot_cdf_list([np.array(list_dis[0]), np.array(list_dis[1]), np.array(list_dis[2])], ['0-10', '10-20', '>20'])
    plt.show()
    # print("1D range: ", range_errs)
    # print("AOA range: ", AOA_err)
    # print("2D range: ", err_2d)