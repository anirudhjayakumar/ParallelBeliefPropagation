import numpy as np
import scipy as sp
import scipy.io
import scipy.ndimage
import scipy.misc
from scipy import ndimage
import matplotlib.pyplot as plt
import matplotlib.image as mpimg
import matplotlib.cm as cm

def process_output(infile, imgfile):
    '''
    Transforms the output of the Charm++ code back
    into an image.
    '''

    #----First, regenerate blurry image----
    #Read in image
    mat = mpimg.imread(imgfile)[:, :, 0]

    #Upsample
    mat = sp.ndimage.zoom(mat, 2, order=3)

    #Break image into blocks after subtracting extra pixels
    M = 5
    N = 5
    endx = -(mat.shape[0] % M) + mat.shape[0]
    endy = -(mat.shape[1] % N) + mat.shape[1]
    mat = mat[:endx, :endy]

    #Contrast normalize the image somewhere

    #----Then, put together output of Charm code----
    with open(infile) as f:
        nrows, ncols, bdim = map(float, f.readline().strip().split())
        bdim = int(np.sqrt(bdim))
        mat2 = np.zeros((nrows*bdim, ncols*bdim))
        for line in f:
            nums = map(float, line.strip().split())
            row = int(nums[0])
            col = int(nums[1])
            data = np.array(nums[2:]).reshape((bdim, bdim))

            mat2[row*bdim:(row+1)*bdim, col*bdim:(col+1)*bdim] = data

    plt.figure()
    plt.imshow(mat, cmap=cm.Greys_r)
    plt.title('Before')

    mat = mat + mat2

    plt.figure()
    plt.imshow(mat, cmap=cm.Greys_r)
    plt.title('After')
    plt.show()
    
    return mat


def main():
    inname = 'block_input.txt'
    imgname = 'cat.png'
    process_output(inname, imgname)
    

if __name__ == '__main__': main()
