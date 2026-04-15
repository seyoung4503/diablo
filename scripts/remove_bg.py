#!/usr/bin/env python3
"""Remove background from sprite images using flood fill from corners.
Converts JPG sprites to PNG with transparent background."""

import sys
from PIL import Image
import numpy as np
from collections import deque

def flood_fill_mask(img_array, start_x, start_y, tolerance=40):
    """Flood fill from a point, returning a mask of connected similar pixels."""
    h, w = img_array.shape[:2]
    mask = np.zeros((h, w), dtype=bool)
    start_color = img_array[start_y, start_x].astype(int)

    queue = deque([(start_x, start_y)])
    mask[start_y, start_x] = True

    while queue:
        x, y = queue.popleft()
        for dx, dy in [(-1,0),(1,0),(0,-1),(0,1)]:
            nx, ny = x + dx, y + dy
            if 0 <= nx < w and 0 <= ny < h and not mask[ny, nx]:
                pixel = img_array[ny, nx].astype(int)
                diff = np.abs(pixel - start_color)
                if np.all(diff < tolerance):
                    mask[ny, nx] = True
                    queue.append((nx, ny))
    return mask

def remove_background(input_path, output_path, tolerance=40):
    """Remove background by flood filling from all 4 corners."""
    img = Image.open(input_path).convert('RGB')
    arr = np.array(img)
    h, w = arr.shape[:2]

    # Flood fill from 4 corners
    bg_mask = np.zeros((h, w), dtype=bool)
    corners = [(0, 0), (w-1, 0), (0, h-1), (w-1, h-1)]
    for cx, cy in corners:
        corner_mask = flood_fill_mask(arr, cx, cy, tolerance)
        bg_mask |= corner_mask

    # Also flood fill from edge midpoints for better coverage
    edge_points = [(w//2, 0), (w//2, h-1), (0, h//2), (w-1, h//2)]
    for ex, ey in edge_points:
        edge_mask = flood_fill_mask(arr, ex, ey, tolerance)
        bg_mask |= edge_mask

    # Create RGBA image
    rgba = np.zeros((h, w, 4), dtype=np.uint8)
    rgba[:,:,:3] = arr
    rgba[:,:,3] = 255  # fully opaque
    rgba[bg_mask, 3] = 0  # background = transparent

    # Smooth edges: partially transparent pixels near the boundary
    from scipy.ndimage import binary_dilation
    dilated = binary_dilation(bg_mask, iterations=1)
    edge = dilated & ~bg_mask
    rgba[edge, 3] = 128  # semi-transparent edge

    result = Image.fromarray(rgba, 'RGBA')
    result.save(output_path)
    print(f"OK: {output_path} ({result.size[0]}x{result.size[1]})")

if __name__ == '__main__':
    if len(sys.argv) < 3:
        print(f"Usage: {sys.argv[0]} input.jpg output.png [tolerance]")
        sys.exit(1)

    tol = int(sys.argv[3]) if len(sys.argv) > 3 else 40
    try:
        remove_background(sys.argv[1], sys.argv[2], tol)
    except ImportError:
        # scipy not available, skip edge smoothing
        print("Note: scipy not available, skipping edge smoothing")
        img = Image.open(sys.argv[1]).convert('RGB')
        arr = np.array(img)
        h, w = arr.shape[:2]
        bg_mask = np.zeros((h, w), dtype=bool)
        for cx, cy in [(0,0),(w-1,0),(0,h-1),(w-1,h-1),(w//2,0),(w//2,h-1),(0,h//2),(w-1,h//2)]:
            bg_mask |= flood_fill_mask(arr, cx, cy, tol)
        rgba = np.zeros((h, w, 4), dtype=np.uint8)
        rgba[:,:,:3] = arr
        rgba[:,:,3] = 255
        rgba[bg_mask, 3] = 0
        Image.fromarray(rgba, 'RGBA').save(sys.argv[2])
        print(f"OK: {sys.argv[2]}")
