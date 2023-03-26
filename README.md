# Mandelbrot GPU

Render mandelbrot fractals on the GPU using openCL. Supports double precision using fixed point decimal calculations.

## Building

```bash
make
./a.out
```

## Configuration
The program can be configured with the `config.cfg` file,

```cfg
# Resolution
width = 1080
height = 720

# Number of iterations per frame
steps = 20

# Number of anti-aliasing samples in the x and y directions
anti_alias_samples = 2

# Technical options, can be ignored
profile = false
verbose = true
useGpu = true

# Starting position
scale = 7.01041481809828e-12
center_x = -0.124342549132596
center_y = 0.989003740008400
theta = 0.394995
```

## Controls
 - You can zoom in by selecting a region with the mouse, and pressing `a` while holding down the mouse button. 
 - Shift the selected region with the arrow keys
 - Increase or decrease the zoom by 20% with the `[` and `]` keys
 - The rendered texture can be zoomed independently, with the `w` and `s` keys. Center a point by right clicking.

## Rendering images
By pressing the `W` key, the texture is written to a csv file. This can then be transformed to a png with the `convert_raw_images.py` script. Install the dependencies by running

```bash
pip install -r requirements.txt
```
