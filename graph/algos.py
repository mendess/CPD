import fileinput
from matplotlib import pyplot as plt
import threading

def matrix_b(i_range, j_range, k_range):
    for i in range(*i_range):
        for j in range(*j_range):
            for k in range(*k_range):
                plt.plot(i, j, c='orange', marker='*')
                plt.plot(k, i, c='blue', marker='v')
                plt.plot(k, j, c='red', marker='^')

def next_iter_lt(i_range, j_range, k_range):
    for k in range(*k_range):
        for i in range(*i_range):
            for j in range(*j_range):
                plt.plot(i, j, c='orange', marker='^')
            plt.plot(k, i, c='blue', marker='v')

def next_iter_r(i_range, j_range, k_range):
    for k in range(*k_range):
        for j in range(*j_range):
            for i in range(*i_range):
                plt.plot(i, j, c='orange', marker='^')
            plt.plot(k, j, c='blue', marker='v')



header = []
for line in fileinput.input():
    header.append(line.strip())
    if len(header) == 4:
        break

t = header[3].split()
i = (0, int(t[0]))
j = (0, int(t[1]))
k = (0 + 2, int(int(header[2]) / 2) + 2)
print("Using:")
print("i:", i)
print("j:", j)
print("k:", k)
args = [i, j, k]

def plot(title):
    plt.xlim([-1, int(t[0])])
    plt.ylim([-1, int(t[1])])
    plt.title(title)
    plt.xlabel('i : linhas')
    plt.ylabel('j : colunas')
    plt.show()

matrix_b(*args)
plot('b')

# Normal
# access_l(*args)
# plot('l')
# access_r(*args)
# plot('r')

# L Transposed
next_iter_lt(*args)
plot('lt')
next_iter_r(*args)
plot('r')

# R Transposed
# access_l_rt(*args)
# plot('l rt')
# access_rt(*args)
# plot('rt')
