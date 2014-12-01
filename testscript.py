import numpy as np
import scipy as sp
import scipy.io
import scipy.ndimage
import scipy.misc
from scipy import ndimage
import matplotlib.pyplot as plt
import matplotlib.image as mpimg

def blockshaped(arr, nrows, ncols):
    """
    Return an array of shape (n, nrows, ncols) where
    n * nrows * ncols = arr.size

    If arr is a 2D array, the returned array should look like n subblocks with
    each subblock preserving the "physical" layout of arr.
    Taken from Stackoverflow.com.
    http://stackoverflow.com/questions/16856788/slice-2d-array-into-smaller-2d-arrays
    """
    h, w = arr.shape
    return (arr.reshape(h//nrows, nrows, -1, ncols)
               .swapaxes(1,2)
               .reshape(-1, nrows, ncols))


fname = 'cat.jpg'
#Read in image (mat file in this case)
mat = mpimg.imread(fname)[:, :, 0]

#Blur initial image
blurred = sp.ndimage.uniform_filter(mat)

#Select only even indices to downsample image
small = blurred[::2, ::2]

#Use cubic interpolation to upsample image
newbig = sp.ndimage.zoom(small, 2, order=3)
lowbig = ndimage.gaussian_filter(mat, 3)
newbig = newbig - lowbig

#Break upsampled image into blocks after subtracting extra pixels
PATCH_SIZE = 5
endx = -((newbig.shape[0]-2) % PATCH_SIZE) + newbig.shape[0]
endy = -((newbig.shape[1]-2) % PATCH_SIZE) + newbig.shape[1]
newbig = newbig[1:endx-1, 1:endy-1]
blocks = blockshaped(newbig, PATCH_SIZE, PATCH_SIZE)

#Make high frequency image and break it into blocks
lowpass = ndimage.gaussian_filter(mat, 3)
highpass = mat - lowpass
highpass = highpass[:endx, :endy]

#Break high frequency image into blocks
shp = blocks.shape
numblocksx = (endx-2)/shp[1]
numblocksy = (endy-2)/shp[2]
highblocks = np.zeros((shp[0], shp[1]+2, shp[2]+2))
for i in range(numblocksx):
    for j in range(numblocksy):
        sx = i * shp[1]
        ex = sx + shp[1] + 1
        sy = j * shp[2]
        ey = sy + shp[2] + 1
        highblocks[i*numblocksy + j] = highpass[sx:ex+1, sy:ey+1]

#Contrast normalize the image somewhere
#Cut out the most low-frequency patches

#Write output to file
#for i in range
