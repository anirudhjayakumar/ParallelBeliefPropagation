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
shp = blocks.shape
highblocks = np.zeros((shp[0], shp[1]+2, shp[2]+2))
for i in range(0, highblocks.shape[1]):
    starti = i*PATCH_SIZE
    endi = starti + PATCH_SIZE + 2
    for j in range(0, highblocks.shape[2]):
        startj = j*PATCH_SIZE
        endj = startj + PATCH_SIZE + 2
        highblocks[i*highblocks.shape[1] + j] = \
            highpass[starti:endi, startj:endj]

#Contrast normalize the image somewhere
#MxM vs NxN
#Cut out the most low-frequency patches
