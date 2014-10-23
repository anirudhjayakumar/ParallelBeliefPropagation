import numpy as np
import scipy as sp
import scipy.io
import scipy.ndimage
import scipy.misc
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
PATCH_SIZE = 4
endx = -(newbig.shape[0] % PATCH_SIZE) + newbig.shape[0]
endy = -(newbig.shape[1] % PATCH_SIZE) + newbig.shape[1]
newbig = newbig[:endx, :endy]
blocks = blockshaped(newbig, PATCH_SIZE, PATCH_SIZE)

#Contrast normalize the image somewhere
#MxM vs NxN
#Highpass filter or difference?
#Cut out the most low-frequency patches

#Find the diff patches corresponding to each low-res patch
diffimage = mat[:endx, :endy] - newbig
blocks = blockshaped(diffimage, PATCH_SIZE, PATCH_SIZE)

#Output the patch pairs into binary files
