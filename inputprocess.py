import numpy as np
import scipy as sp
import scipy.io
import scipy.ndimage
import scipy.misc
from scipy import ndimage
import matplotlib.pyplot as plt
from PIL import Image

def contrast_normalize(m):
    avg = np.average(m)
    m = m - avg
    m[np.where(m>255)] = 255
    m[np.where(m<0)] = 0

    return m


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


def process_input(infile, outfile):
    #Read in image (mat file in this case)
    im = Image.open(infile)
    mat = np.array(im, dtype=float)[:, :, 0]

    #Upsample and high-pass filter
    mat = sp.ndimage.zoom(mat, 2, order=3)
    lowpass = ndimage.gaussian_filter(mat, 3)
    mat = mat - lowpass

    #Break image into blocks after subtracting extra pixels
    M = 5
    N = 5
    endx = -(mat.shape[0] % M) + mat.shape[0]
    endy = -(mat.shape[1] % N) + mat.shape[1]
    mat = mat[:endx, :endy]
    blocks = blockshaped(mat, M, N)
    for i in range(len(blocks)):
        blocks[i] = contrast_normalize(np.copy(blocks[i]))

    #Contrast normalize the image somewhere

    #Write output to file
    nrows = endx / M
    ncols = endy / N
    m2 = M*N
    with open(outfile, 'w') as f:
        header = '%d %d %d %d\n' % (nrows*ncols,nrows, ncols,m2)
        f.write(header)
        for i in range(nrows):
            for j in range(ncols):
                s = map(str, blocks[i*ncols + j].flatten())
                f.write('%d %d %s\n' % (i, j, " ".join(s)))


def main():
    inname = 'small_input/input_600_450.jpg'
    outname = 'inputblocks_600_450.txt'
    process_input(inname, outname)
    

if __name__ == '__main__': main()
