import numpy as np
import scipy as sp
import scipy.io
import scipy.ndimage
import scipy.misc
from scipy import ndimage
import matplotlib.pyplot as plt
import matplotlib.image as mpimg

def process_output(infile):
    '''
    Transforms the output of the Charm++ code back
    into an image.
    '''

    #Contrast normalize the image somewhere

    #Write output to file
    with open(infile) as f:
        nrows, ncols, bdim = map(float, f.readline().strip().split())
        mat = np.zeros((nrows*bdim, ncols*bdim))
        for line in f:
            nums = map(float, line.strip().split())
            row = int(nums[0])
            col = int(nums[1])
            #data = np.array(nums[2:]).reshape((bdim, bdim))
            data = np.array(nums[3:]).reshape((bdim, bdim))

            mat[row*bdim:(row+1)*bdim, col*bdim:(col+1)*bdim] = data

    plt.imshow(mat)
    plt.show()
    
    return mat


def main():
    inname = 'block_input.txt'
    process_output(inname)
    

if __name__ == '__main__': main()
