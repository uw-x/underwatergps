import numpy as np 
import glob
from Data_Loader import read_IMU, read_mic_t
import matplotlib.pyplot as plt 

if __name__ == "__main__":
    fname = "E4"
    traj0 = np.load("video/" + fname + "_Trim_traj0.npy")
    traj1 = np.load("video/" + fname + "_Trim_traj1.npy")    


    raw_folder1 = "raw_data/p1/" +  fname
    raw_folder2 = "raw_data/p2/" +  fname

    f1_lists = glob.glob(raw_folder1 + "/*.txt")
    f2_lists = glob.glob(raw_folder2 + "/*.txt")    

    for f in f2_lists:
        if "acc.txt" in f:
            time_acc, data_acc = read_IMU(f)
            plt.figure()
            plt.plot(data_acc[:, 0])
            plt.plot(data_acc[:, 1])
            plt.plot(data_acc[:, 2])
            plt.show()

