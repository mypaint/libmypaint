import numpy as np
import matplotlib.pyplot as plt

dab = plt.imread('parametric_dab.png')

print dab.shape

l = dab[500, :, 3]
#plt.plot(l, label="opacity")

r = np.hstack((np.linspace(1, 0, 500), np.linspace(0, 1, 500)[1:]))
#plt.plot(r, label="$r$")

rr = r**2
o = 1.0-rr
#plt.plot(o, label="$1-r^2$")

for i in [1, 2]:
    plt.figure(i)
    hardness = 0.3
    for hardness in [0.1, 0.3, 0.7]:
        if i == 2:
            rr = np.linspace(0, 1, 1000)
        opa = rr.copy()
        opa[rr < hardness] = rr[rr < hardness] + 1-(rr[rr < hardness]/hardness)
        opa[rr >= hardness] = hardness/(1-hardness)*(1-rr[rr >= hardness])
        plt.plot(opa, label="h=%.1f" % hardness)
        if i == 2:
            plt.xlabel("$r^2$")
            plt.legend(loc='best')
        else:
            plt.xlabel("$d$")
            plt.legend(loc='lower center')

    plt.ylabel('pixel opacity')
    plt.xticks([500], [0])
    plt.title("Dab Shape (for different hardness values)")

plt.show()
