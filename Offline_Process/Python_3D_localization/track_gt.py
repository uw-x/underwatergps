from pickle import NONE
import numpy as np
import cv2
import cv2.aruco as aruco

import matplotlib.pyplot as plt

Desk_corner = np.array([ # 152.2, 60.8
    [0 ,0, 0],
    [40, 0, 0],
    [80, 0, 0],
    [120, 0, 0],
    [152.2, 0, 0],
    [152.2, 30, 0],
    [152.2, 60.8, 0],
    [120, 60.8, 0],
    [80, 60.8, 0],
    [40, 60.8, 0],
    [0, 60.8, 0],
    [0, 30, 0]
])
N_achor = Desk_corner.shape[0]

resolution = 2
Desk_L = 152.2#result3
Desk_W = 60.8
N_x = int(np.round(Desk_L/resolution)) + 1
N_y = int(np.round(Desk_W/resolution)) + 1
N_grids =  N_x * N_y


def map_img2real(points, grid_pixel, grids_cm):
    # print(grid_pixel.shape)
    grid_pixel2 = grid_pixel.reshape((N_y*N_x, 2))
    diff = np.sum(np.abs(grid_pixel2 - points), axis = 1)
    min_id = np.argmin(diff)
    idx = int(min_id//N_x)
    idy = (min_id%N_x)

    if idx <= 0:
        min_idx = 0
    else:
        min_idx = idx - 1
    if idx >= N_y - 1:
        max_idx =  N_y - 1
    else:
        max_idx = idx + 1

    if idy <= 0:
        min_idy = 0
    else:
        min_idy = idy - 1
    if idy >= N_x - 1:
        max_idy =  N_x - 1
    else:
        max_idy = idy + 1

    box_border = np.array([
        grid_pixel[min_idx, min_idy],
        grid_pixel[max_idx, min_idy],
        grid_pixel[max_idx, max_idy],
        grid_pixel[min_idx, max_idy],
    ])
    # print(box_border)

    cm_border = np.array([
        grids_cm[min_idx, min_idy],
        grids_cm[max_idx, min_idy],
        grids_cm[max_idx, max_idy],
        grids_cm[min_idx, max_idy],
    ])
    # print(cm_border)
    m2, _ =  cv2.estimateAffinePartial2D(box_border.astype(np.float32), cm_border[:, :2].astype(np.float32))
    trans2 = m2[:,2:]
    Rot = m2[:,0:2]

    points = np.array([points])
    loc = np.matmul(Rot, np.transpose(points) ) + trans2
    loc = loc.ravel()

    return loc
    
    

def detect_aurocode(frame, grid_pixel, grids_cm):
    gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)

    dictionary = cv2.aruco.getPredefinedDictionary(cv2.aruco.DICT_4X4_250)
    parameters =  cv2.aruco.DetectorParameters()
    detector = cv2.aruco.ArucoDetector(dictionary, parameters)
    
    corners, ids, rejectedCandidates = detector.detectMarkers(frame)
    if ids is None:
        return None, None
    frame_markers = aruco.drawDetectedMarkers(frame.copy(), corners, ids)
    # print(ids)
    new_point = {}
    for i in range(len(ids)):
        _id = ids[i][0]
        if _id > 2:
            continue
        corner =  corners[i][0, ...]
        ref_pos = np.mean(corner[0:2, :], axis=0)
        mic_pos = np.mean(corner[2:4, :], axis=0)
        mic_pos = mic_pos + 0.1*(mic_pos - ref_pos)
        rela_loc = map_img2real(mic_pos, grid_pixel, grids_cm)
        new_point[_id] = rela_loc
        frame_markers = cv2.circle(frame_markers,(int(mic_pos[0]), int(mic_pos[1])), 5, (0, 0, 255), -1)
        # print(corner)

    
    return frame_markers, new_point
    # corners = corners0[0][0]
    # real_pos = []
    # pixel_pos = []
    # for i in range(0, corners.shape[0]):
    #     real_index = find_point_2D(corners[i, :], grid_pixel, last_pos)     
    #     if real_index[0] == -1:
    #         return last_speaker, last_Real, last_pos, corners0

    #     real_point = grids_cm[real_index[0], real_index[1], :2]
    #     real_pos.append(real_point)
    #     pixel_pos.append([real_index[0], real_index[1]])
    #     #frame = cv2.circle(frame,(corners[i,0], corners[i,1]), point_size+1, [255, 0, 0], thickness)
    # real_pos = np.array(real_pos)
    # center_pos = np.mean(real_pos, axis = 0)
    
    # speaker_pos = loc_speaker(real_pos)

    # pixel_pos = np.array(pixel_pos)
    # center_pixel = np.mean(pixel_pos, axis = 0)
    # center_pixel = center_pixel.astype(np.int32)
    # #print(real_pos)
    # #print(center_pos)

    # return speaker_pos, center_pos, center_pixel, corners0


if __name__ == "__main__":
    mtx = np.load( 'mtx_usb.npy')
    dist = np.load('dist_usb.npy')
    Img_corner = np.load("img_anchor.npy")
    retval,rvec,tvec = cv2.solvePnP(Desk_corner, Img_corner, mtx, dist)
    fname = "video/E4_Trim"
    cap = cv2.VideoCapture( fname + '.mp4')

    grids_cm = np.zeros((N_grids,3), np.float32)
    grids_cm[:, :2] = np.mgrid[0:N_x,0:N_y].T.reshape(-1,2)*resolution

    grid_pixel, _ = cv2.projectPoints(grids_cm, rvec, tvec, mtx, dist)
    grid_pixel = grid_pixel[:,0,:]
    grids_cm = grids_cm.reshape((N_y, N_x, 3))
    grid_pixel = grid_pixel.reshape((N_y, N_x, 2))

    
    imgpoints2= np.around(grid_pixel)
    imgpoints2 = imgpoints2.astype(np.int32)
    ret, frame = cap.read()
    for i in range(0, imgpoints2.shape[0]):
        for j in range(0, imgpoints2.shape[1]):
                frame = cv2.circle(frame,(int(imgpoints2[i,j,0]), int(imgpoints2[i,j,1])), 2, (0, 0, 255), -1)
    frame = cv2.circle(frame,(int(imgpoints2[0,0,0]), int(imgpoints2[0,0,1])), 2, (0, 0, 0), -1)
    # cv2.imshow("ss", frame)
    # cv2.waitKey(-1)

    trajs = [[], []]
    
    t = 0
    while(cap.isOpened()):
        # Capture frame-by-frame
        ret, frame = cap.read()
        if ret == False:
            break
        ret_img, new_point = detect_aurocode(frame, grid_pixel, grids_cm)
        
        if ret_img is not None:
            for key in new_point.keys():
                trajs[key].append([t, new_point[key][0], new_point[key][1]])

            cv2.imshow('frame', ret_img)
            if cv2.waitKey(1) & 0xFF == ord('q'):
                break
        t += 1
        
    cv2.destroyAllWindows()
    traj0 = np.array(trajs[0])
    traj1 = np.array(trajs[1])
    np.save(fname + "_traj0.npy", traj0)
    np.save(fname + "_traj1.npy", traj1)
    plt.figure()
    plt.plot(traj0[:, 1], traj0[:, 2])
    plt.plot(traj1[:, 1], traj1[:, 2])
    plt.show()