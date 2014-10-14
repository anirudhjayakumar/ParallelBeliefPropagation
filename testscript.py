import numpy as np
import scipy as sp
import scipy.io
import scipy.signal
import scipy.ndimage
from PIL import Image
import matplotlib.pyplot as plt

mat = sp.io.loadmat("digits.mat")
mat = mat['d'][:,0].reshape(28,28).T
plt.imshow(mat, interpolation='none')
plt.show()

blurred = sp.ndimage.uniform_filter(mat)
plt.imshow(blurred, interpolation='none')
plt.show()

small = blurred[::2, ::2]
plt.imshow(small, interpolation='none')
plt.show()

