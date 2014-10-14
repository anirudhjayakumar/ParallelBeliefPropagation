import numpy as np
import scipy as sp
import scipy.io
import scipy.ndimage
import scipy.misc
import matplotlib.pyplot as plt

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

#Read in image (mat file in this case)
mat = sp.io.loadmat("digits.mat")
mat = mat['d'][:,0].reshape(28,28).T
plt.imshow(mat, interpolation='none')
plt.show()

#Blur initial image
blurred = sp.ndimage.uniform_filter(mat)
plt.imshow(blurred, interpolation='none')
plt.show()

#Select only even indices to downsample image
small = blurred[::2, ::2]
plt.imshow(small, interpolation='none')
plt.show()

#Use cubic interpolation to upsample image
newbig = sp.ndimage.zoom(small, 2, order=3)
plt.imshow(newbig, interpolation='none')
plt.show()

#Break upsampled image into blocks
PATCH_SIZE = 7
blocks = blockshaped(small, PATCH_SIZE, PATCH_SIZE)
