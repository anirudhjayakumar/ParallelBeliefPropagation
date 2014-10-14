import numpy as np
import scipy as sp
import scipy.io
import scipy.ndimage
import scipy.misc
import matplotlib.pyplot as plt

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
