# Pixxcraft
A small, MS-Paint style widget that allows you to create small, animated, frame-by frame paintings. It runs on an ESP-32 and a touchscreen TFT.

BOM:

To flash the board, go into "software", and flash "sketch_oct27a.ino" to the ESP-32 like you would a regular board.

There are 6 predefined colors, but you can always make your own one using the RGB selector. The final 2 buttons are erasers and screen clearers. Up top you can select different brush sizes, and create new colors as I mentioned before. Once you have created a drawing, tap "Frame +" to create a new frame. I'll copy over your last one, but if you don't want that, just tap "CLR" when the new frame is created. Finally, up top you can go through your past frames and change things up, or change the framerate, or speed at which the frames play back at. Then, tap the play button to see your animation.

Interface on the ESP-32 (simulator):
<img width="980" height="631" alt="Screenshot 2026-10-31 235832" src="https://github.com/user-attachments/assets/2f9ad4f4-68fc-4961-b7a5-21ac5e6ccfe7" />

PCB Render:
<img width="771" height="492" alt="image" src="https://github.com/user-attachments/assets/e813f028-0ea6-49b3-a738-827bc1f8ad9b" />
