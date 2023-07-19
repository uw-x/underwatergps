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
Img_corner = np.ones((N_achor, 2))*100
anchor_id = 0

def trackbar_x(x):
    """Trackbar callback function."""
    text = f'Trackbar: {x}'
    Img_corner[anchor_id, 0] = x


def trackbar_y(x):
    """Trackbar callback function."""
    text = f'Trackbar: {x}'
    Img_corner[anchor_id, 1] = x
    


def calibrate_ex_param(img, mtx, dist):
    global anchor_id, Img_corner, Desk_corner
    SCALE_DOWN = 0.8
    W, H, C = img.shape

    print(W,H,C)
    for i in range(N_achor):
        anchor_id = i
        img_resize = cv2.resize(img, None, fx = SCALE_DOWN, fy = SCALE_DOWN)
        cv2.imshow('select', img_resize)
        # setting mouse handler for the image
        # cv2.setMouseCallback('select', click_event)
        # wait for a key to be pressed to exit
        cv2.createTrackbar('x', 'select', 0, H, trackbar_x)
        cv2.createTrackbar('y', 'select', 0, W, trackbar_y)
        k = 0
        while(k != ord('q')):
            k = cv2.waitKey(0)
            if(k == ord('\r') or k == ord('\n')):
                overlay = img.copy()
                overlay2 = img.copy()
                
                cv2.circle(overlay, (int(Img_corner[anchor_id, 0]), int(Img_corner[anchor_id, 1])), 40, (0, 0, 255), -1)
                alpha = 0.4  # Transparency factor.
                # Following line overlays transparent rectangle over the image
                image_new = cv2.addWeighted(overlay2, alpha, overlay, 1 - alpha, 0)
                cv2.circle(image_new, (int(Img_corner[anchor_id, 0]), int(Img_corner[anchor_id, 1])), 5, (0, 0, 0), -1)
                img_resize = cv2.resize(image_new, None, fx = SCALE_DOWN, fy = SCALE_DOWN)
                cv2.imshow('select', img_resize)

            if(k == ord('q')):
                    break

        # close the window
        cv2.destroyAllWindows()

    Img_corner= Img_corner.astype('float32')
    print(Img_corner)
    np.save("img_anchor.npy", Img_corner)

if __name__ == "__main__":
    mtx = np.load( 'mtx_usb.npy')
    dist = np.load('dist_usb.npy')
    vi_cap = cv2.VideoCapture("video/E1.mov")
    vi_cap.set(cv2.CAP_PROP_POS_FRAMES, 60*30)
    ret, img = vi_cap.read()
    print(ret)
    print(img.shape)
    if ret != True:
        raise KeyboardInterrupt
    calibrate_ex_param(img, mtx, dist)