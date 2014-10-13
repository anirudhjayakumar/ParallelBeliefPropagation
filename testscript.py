import numpy as np
import scipy as sp
import scipy.io
from PIL import Image
import matplotlib.pyplot as plt

mat = sp.io.loadmat("digits.mat")
mat = mat['d'][:,0].reshape(28,28).T
imgplt = plt.imshow(mat, interpolation='none')
plt.show()
