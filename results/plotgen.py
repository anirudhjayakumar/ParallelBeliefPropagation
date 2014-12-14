import numpy as np
import scipy as sp
import matplotlib.pyplot as plt

def applySettings(title):
    plt.xlabel('Cores')
    plt.ylabel('Execution Time (sec)')
    plt.title(title)
    plt.legend(loc=0)
#8 processors per node, 7ppn, 1 comm

f = open('scalability_nums.txt', 'r')
totlines = f.readline().split('\r')
for run in range(4):
    plt.figure()
    line = totlines.pop(0).strip().split(',')
    size_labels = line[:2]
    line = totlines.pop(0).strip().split(',')
    sizes = map(int,line[:2])
    labels = totlines.pop(0).strip().split(',')
    data = []
    for i in range(4):
        data.append(map(float,totlines.pop(0).strip().split(',')))
    data = np.array(data)
    title = 'Strong Scaling with '
    title += size_labels[0] + ' = ' + str(sizes[0]) + ', ' + size_labels[1] + ' = ' + str(sizes[1])
    plt.loglog(data[:,0], data[:,1], 'o-', label=labels[1], lw=3, basex=2, basey=2)
    plt.loglog(data[:,0], data[:,2], 'o-', label=labels[2], lw=3, basex=2, basey=2)
    plt.loglog(data[:,0], data[:,3], 'o-', label=labels[3], lw=3, basex=2, basey=2)
    slope1x = data[:,0]
    slope1y = data[:,0][::-1]
    maxbase = np.log2(data[0,2])
    xbase = np.log2(data[-1,0])
    plt.loglog(slope1x, slope1y*2**(maxbase-xbase-1), label='Linear Slope', lw=2, basex=2, basey=2)
    applySettings(title)
    fname = str(sizes[0]) + '_' + str(sizes[1]) + '.pdf'
    plt.savefig(fname)
    if run != 3:
        line = totlines.pop(0)
        
    
