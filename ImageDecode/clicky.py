import json
import matplotlib.pyplot as plt
import numpy as np
import cv2

# Load an image (replace 'image.jpg' with your actual image file)
image_path = '/home/michal/code/MandeyeLedTimestamp/ImageDecode/20250311_193228.jpg'
image = cv2.imread(image_path)
image = cv2.cvtColor(image, cv2.COLOR_BGR2RGB)  # Convert to RGB for Matplotlib


# Data storage
click_data = []
bit_counter = 0

# Click event function
def on_click(event):
    global bit_counter
    if event.button == 1:  # Left click to add
        if event.xdata is not None and event.ydata is not None:
            click_data.append({
                "bit": bit_counter,
                "location": [int(event.xdata), int(event.ydata)],
                "radius": 10
            })
            bit_counter += 1
            print(f"Captured: {click_data[-1]}")

            # Redraw with a marker
            plt.scatter(event.xdata, event.ydata, c='red', s=50)
            plt.draw()
    elif event.button == 3:  # Right click to undo
        if click_data:
            removed = click_data.pop()
            bit_counter -= 1
            print(f"Removed: {removed}")
            
            # Redraw the image
            ax.clear()
            ax.imshow(image)
            ax.set_title("Click to select points")
            for point in click_data:
                ax.scatter(point["location"][0], point["location"][1], c='red', s=50)
            plt.draw()

def on_close(event):
    # Save to JSON when the window is closed
    with open("click_data.json", "w") as f:
        json.dump(click_data, f, indent=4)
    print("Saved click_data.json")

# Plot the image
fig, ax = plt.subplots()
ax.imshow(image)
ax.set_title("Click to select points")
fig.canvas.mpl_connect("button_press_event", on_click)
fig.canvas.mpl_connect("close_event", on_close)
plt.show()
