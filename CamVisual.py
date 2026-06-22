#this code has two mode to display images, Box and classification
#BOX parses the pixel when the central pixel changes, so use this mode when you want per box update
#Classification parses the pixels by row, so use this mode when you send row values.

import serial
import numpy as np
import cv2
import matplotlib.pyplot as plt
from matplotlib.animation import FuncAnimation


ser = serial.Serial("COM3", 115200, timeout=1)

mode = None  # "BOX" or "CLASSIFICATION"


box_img = np.zeros((32, 32), dtype=np.float32)
BOX_SIZE = 3
DISPLAY_SIZE = 224

class_img = np.zeros((32, 32), dtype=np.float32)
current_row = 0


fig, ax = plt.subplots()
im = ax.imshow(np.zeros((32, 32)), cmap='gray', vmin=0, vmax=255)
ax.axis('off')



def process_box(img):

    img = img.astype(np.float32)

    img = cv2.GaussianBlur(img, (3, 3), 0)

    img = img - np.mean(img)
    p5 = np.percentile(img, 5)
    p95 = np.percentile(img, 95)
    img = np.clip(img, p5, p95)

    img = (img - img.min()) / (img.max() - img.min() + 1e-6)

    img = img **6

   

    return img


def process_class(img):

    img = img.astype(np.float32)

    img = cv2.GaussianBlur(img, (3, 3), 0)

    img = img - np.mean(img)

    p5 = np.percentile(img, 5)
    p95 = np.percentile(img, 95)
    img = np.clip(img, p5, p95)

    img = (img - img.min()) / (img.max() - img.min() + 1e-6)

    img = img ** 0.6

    return img


def read_serial():

    global mode, current_row, box_img, class_img

    while ser.in_waiting:

        line = ser.readline().decode('utf-8', errors='ignore').strip()

     
        if line == "Initialized Box":
            mode = "BOX"
            print("Switched to BOX")
            continue

        if line == "Initialized Classification":
            mode = "CLASSIFICATION"
            current_row = 0
            print("Switched to CLASSIFICATION")
            continue

        #BOX mode
        if mode == "BOX":

            if line != "BOX":
                continue

            loc = ser.readline().decode().strip()

            try:
                r, c = map(int, loc.split())
            except:
                continue

            block = []

            for _ in range(BOX_SIZE):

                row = ser.readline().decode().strip()

                vals = list(map(float, row.split()))

                if len(vals) == BOX_SIZE:
                    block.append(vals)

            end = ser.readline().decode().strip()

            if end != "END":
                continue

            if len(block) != BOX_SIZE:
                continue

            block = np.array(block, dtype=np.float32)

            x = c * BOX_SIZE
            y = r * BOX_SIZE

            box_img[y:y+BOX_SIZE, x:x+BOX_SIZE] = block


        # Classification mode
        elif mode == "CLASSIFICATION":

            if line in ("FRAME_START", "FRAME_END"):
                continue

            parts = line.split()

            if len(parts) != 32:
                continue

            try:
                row = np.array(list(map(float, parts)))
            except:
                continue

            class_img[current_row] = row
            current_row += 1

            if current_row >= 32:
                current_row = 0



def update(_):

    read_serial()

    global mode

    if mode == "BOX":
        img = process_box(box_img)
        img = cv2.resize(img, (DISPLAY_SIZE, DISPLAY_SIZE), interpolation=cv2.INTER_NEAREST)
        img = (img * 255).astype(np.uint8)

    elif mode == "CLASSIFICATION":
        img = process_class(class_img)
        img = (img * 255).astype(np.uint8)

    else:
        img = np.zeros((32, 32), dtype=np.uint8)
        

    img = cv2.flip(img, 0)

    im.set_array(img)
    return [im]



ani = FuncAnimation(
    fig,
    update,
    interval=30,
    cache_frame_data=False
)

plt.title("Box + Classification Switcher")
plt.show()

ser.close()