import numpy as np
import matplotlib.pyplot as plt
import os
import seaborn as sb
from Weighted_SMACOF import smacof
import matplotlib
colors = {'blue':'#1d7fb8', 'gold': '#e1971b', 'green': '#059567', 'brown': '#d9775a', 'purple':'#ca18de', 'red': '#ac0000' }

# color_lists = ["tab:blue", "tab:orange", "tab:green", "tab:red", "tab:purple", "tab:brown"] 

color_lists = [colors['blue'], colors['red'], colors['green'], colors['brown'], colors['purple'], colors['gold']]
matplotlib.rc('font', serif='Helvetica') 
legend_size = 15
label_size = 15
tick_size = 14

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


def smacof_outlier_detect(dis_measure, outlier = False):
    N = dis_measure.shape[0]
    weight_measure = np.ones((N, N))
    pos_pred, score = smacof(dissimilarities=dis_measure, weight_matrix=weight_measure, n_components=2, metric = True,  max_iter = 1000, eps = 1e-4, random_state=0, n_init=8)
    best_score = max([score * (N*(N-1)  - 1)/ (N*(N - 1)) , 0])
    best_pred = pos_pred   
    best_score_init = best_score
    print("smacof score: ", best_score)

    if outlier and best_score > 1:
        for i in range(N):
            for j in range(i+1, N):
                weight0 = np.copy(weight_measure)
                weight0[i, j] = 0
                weight0[j, i] = 0
                pos_pred0, score0 = smacof(dissimilarities=dis_measure, weight_matrix=weight0, n_components=2, metric = True,  max_iter = 1000, eps = 1e-4, random_state=0, n_init=8)
                print(i, j, score0, best_score)
                if score0 < best_score*0.2 and score0 < best_score:
                    best_score = score0
                    best_pred = pos_pred0
        
        print("best smacof score: ", best_score)
        return best_pred
    else:
        return best_pred



def calculate_dis_matrix(pos):
    num_users = pos.shape[0]
    dis_gt = np.zeros((num_users, num_users))
    for i in range(num_users):
        for j in range(i + 1, num_users):
            tmp_d = np.linalg.norm(pos[i, :] - pos[j, :] )
            dis_gt[i, j] = tmp_d
            dis_gt[j, i] = tmp_d

    return dis_gt




def plot_cdf(data:list, labels: list):
    fig, ax = plt.subplots()


    x_50 = []
    x_90 = []
    mean_list = []
    for i in range(len(data)):
        print(data[i].shape)
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
    # ax.set_yticks([0, 1.05])
    # ax.set_xlim([0, 3])

if __name__ == "__main__":
    exp_folder = "../../../block/"
    gt = np.loadtxt(exp_folder + 'gt.txt', delimiter = ',')
    # load the ground-truth of divers
    gt[:, 0:2] = gt[:, 0:2] - gt[0, 0:2]  
    num_users = gt.shape[0]
    dis_gt = calculate_dis_matrix(gt)
    user_info = np.loadtxt(exp_folder + 'user.txt', delimiter = ',')
    print("Gt: ", dis_gt)
    range_errs = [] 
    AOA_err = []
    err_2d = []
    err_motion = [[], [],  [],  [] , [] ]
    err_dis = [[], [], [], []]

    for exp_num in [ 4, 1,2,3,5,6,7]:
        for f in os.listdir(exp_folder + str(exp_num)):
            if f.endswith(".txt"):
                print(exp_num, f)
                dis_matrix = np.loadtxt(exp_folder + str(exp_num) + '/' +  f, delimiter = ',') # load the measurement pairwise distances
                dis_matrix2 = np.zeros_like(dis_matrix) ### project the distances matrix into 2D plane
                for a in range(0, num_users):
                    for b in range(a+1, num_users):
                        calib_dis = np.sqrt(dis_matrix[a, b]**2 - (gt[a, 2] - gt[b, 2] )**2 )
                        dis_matrix2[a, b] = calib_dis
                        dis_matrix2[b, a] = calib_dis
                ### apply the smacof algorithm to recover the 2D topology based on pairwise ranging
                pos = smacof_outlier_detect(dis_matrix2, True)

                pos = pos - pos[0, :]
                angle = -np.arctan2( pos[int(user_info[1]), 1] , pos[int(user_info[1]), 0])
                R = np.array([
                    [np.cos(angle), np.sin(-angle) ],
                    [np.sin(angle), np.cos(-angle) ]
                ])
                pos2 = (R.dot(pos.T)).T
                
                pos2_flip = np.copy(pos2)
                pos2_flip[:, 1] = pos2_flip[:, 1] * (-1)

                err1 = np.mean(np.linalg.norm(pos2 - gt[:, :2], axis = 1))
                err2 = np.mean(np.linalg.norm(pos2_flip - gt[:, :2], axis = 1))

                if err2 < err1:
                    pos2 = pos2_flip

                pred_3d = np.concatenate([pos2, gt[:, 2:3]], axis = 1)
                pred_dis = calculate_dis_matrix(pred_3d)                
                ### determine the flipping 

                # calculate the 1D and AOA error
                tmp_1d = []
                for a in range(0, num_users):
                    for b in range(a+1, num_users):
                        err_1d = np.abs(dis_gt[a, b] - pred_dis[a, b])
                        range_errs.append(err_1d)
                        tmp_1d.append(err_1d)
                tmp_AOA = []
                for a in range(0, num_users):
                    if a in user_info:
                        continue
                    AOA_gt = np.arctan2(gt[a, 1], gt[a, 0]) 
                    AOA_pred  = np.arctan2(pred_3d[a, 1], pred_3d[a, 0]) 
                    AOA_err.append(np.rad2deg(np.abs(AOA_gt - AOA_pred))% 180)
                    tmp_AOA.append(np.rad2deg(np.abs(AOA_gt - AOA_pred))% 180)
                tmp_2d = []

                for a in range(0, num_users):
                    if a == 0:
                        continue
                    err = np.linalg.norm(gt[a, :2] -pred_3d[a, :2] )
                    leader_dis = np.linalg.norm(gt[a, :2] - gt[0, :2])
                    print(leader_dis)
                    if(leader_dis < 10):
                        err_dis[0].append(err)
                    elif (leader_dis >= 10 and leader_dis <= 15):
                        err_dis[1].append(err)
                    else:
                        err_dis[2].append(err)
                    err_motion[a-1].append(err)

                    err_2d.append(err)
                    tmp_2d.append(err)

                plt.figure()
                for i in range(pos.shape[0]):
                    plt.scatter(pos2[i, 0], pos2[i, 1])
                    plt.scatter(gt[i, 0], gt[i, 1], color = "black",marker = 'x')
                    plt.text(pos2[i, 0]+0.2, pos2[i, 1]+0.2, str(i))
                plt.xlim([-25, 25])
                plt.ylim([-25, 25])
                plt.show()
   