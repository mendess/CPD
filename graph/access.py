import fileinput
from matplotlib import pyplot as plt
import threading

# Normal
def access_l(endi, endj, endk):
    for i in range(*endi):
        for k in range(*endk):
            for j in range(*endj):
                plt.scatter(k, j, c='red', marker='^')
            plt.scatter(i, k, c='blue', marker='v')

def access_r(endi, endj, endk):
    for k in range(*endk):
        for j in range(*endj):
            for i in range(*endi):
                plt.scatter(i, k, c='red', marker='^')
            plt.scatter(k, j, c='blue', marker='v')

# L Transposed
def access_lt(endi, endj, endk):
    for i in range(*endi):
        for k in range(*endk):
            for j in range(*endj):
                plt.scatter(k, j, c='red', marker='^')
            plt.scatter(k, i, c='blue', marker='v')

def access_r_lt(endi, endj, endk):
    for k in range(*endk):
        for j in range(*endj):
            for i in range(*endi):
                plt.scatter(k, i, c='red', marker='^')
            plt.scatter(k, j, c='blue', marker='v')

# R Transposed
def access_l_rt(endi, endj, endk):
    for i in range(*endi):
        for k in range(*endk):
            for j in range(*endj):
                plt.scatter(j, k, c='red', marker='^')
            plt.scatter(i, k, c='blue', marker='v')

def access_rt(endi, endj, endk):
    for k in range(*endk):
        for j in range(*endj):
            for i in range(*endi):
                plt.scatter(i, k, c='red', marker='^')
            plt.scatter(j, k, c='blue', marker='v')


header = []
for line in fileinput.input():
    header.append(line.strip())
    if len(header) == 4:
        break

t = header[3].split()
i = (0, int(t[0]))
j = (0, int(t[1]))
k = (0, int(int(header[2]) / 4))
args = [i, j, k]

def plot(title):
    plt.title(title)
    plt.xlabel('i : linhas')
    plt.ylabel('j : colunas')
    plt.show()

# Normal
# access_l(*args)
# plot('l')
# access_r(*args)
# plot('r')

# L Transposed
access_lt(*args)
plot('lt')
access_r_lt(*args)
plot('r lt')

# R Transposed
# access_l_rt(*args)
# plot('l rt')
# access_rt(*args)
# plot('rt')
