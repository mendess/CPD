import fileinput
from matplotlib import pyplot as plt
import threading


def access_l(endi, endj, endk):
    for i in range(*endi):
        for k in range(*endk):
            for j in range(*endj):
                plt.scatter(k, j, c='red', marker='^')
            plt.scatter(i, k, c='blue', marker='v')

def access_lt(endi, endj, endk):
    for i in range(*endi):
        for k in range(*endk):
            for j in range(*endj):
                plt.scatter(k, j, c='red', marker='^')
            plt.scatter(k, i, c='blue', marker='v')

def access_l_rt(endi, endj, endk):
    for i in range(*endi):
        for k in range(*endk):
            aux = 0
            for j in range(*endj):
                # aux += r[j, k]
                plt.scatter(j, k, c='red', marker='^')
            plt.scatter(i, k, c='blue', marker='v')
            # l = aux

def access_r(endi, endj, endk):
    for k in range(*endk):
        for j in range(*endj):
            for i in range(*endi):
                plt.scatter(i, k, c='red', marker='^')
            plt.scatter(k, j, c='blue', marker='v')

def access_r_t(endi, endj, endk):
    for k in range(*endk):
        for j in range(*endj):
            for i in range(*endi):
                plt.scatter(i, k, c='red', marker='^')
            plt.scatter(j, k, c='blue', marker='v')

def access_r_at(endi, endj, endk):
    for k in range(*endk):
        for j in range(*endi):
            for i in range(*endj):
                plt.scatter(i, k, c='red', marker='^')
            plt.scatter(k, j, c='blue', marker='v')

def access_r_lt(endi, endj, endk):
    for k in range(*endk):
        for j in range(*endj):
            for i in range(*endi):
                plt.scatter(k, i, c='red', marker='^')
            plt.scatter(k, j, c='blue', marker='v')

header = []
for line in fileinput.input():
    header.append(line.strip())
    if len(header) == 4:
        break

k = int(header[2])
t = header[3].split()
i = int(t[0])
j = int(t[1])
# access_l((0, 4), (0, j), (0, 1))
# plt.title('l')
# plt.show()
# access_l_rt((0, 4), (0, j), (0, 1))
# plt.title('l rt')
# plt.show()

access_lt((0, 4), (0, j), (0, 2))
plt.title('lt')
plt.show()
access_r_lt((0, 4), (0, j), (0, 2))
plt.title('r lt')
plt.show()
# access_r((0, 1), (0, j), (0, k))
# plt.title('r')
# plt.show()
# access_r_t((0, 1), (0, j), (0, k))
# plt.title('rt')
# plt.show()
# access_r_at((0, 1), (0, j), (0, k))
# plt.title('r at')
# plt.show()
