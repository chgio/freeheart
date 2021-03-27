import pandas as pd
import numpy as np
import matplotlib.pyplot as plt

infile = "test/data/scope_1.csv"
df = pd.read_csv(infile, index_col=False)

t = df.time.to_list()
vSens = df.ch1.to_list()
vAsps = df.ch2.to_list()

alpha = 0.15
vLag = 0
vLags = []
for i, vAsp in enumerate(vAsps):
    vLag = (alpha) * vAsp + (1-alpha) * vLag
    vLags.append(vLag)

rollingAvg = 0
vNorms = []
for i, vLag in enumerate(vLags):
    rollingAvg = (rollingAvg + vLag) / (i+1)
    vNorm = vLag - rollingAvg
    vNorms.append(vNorm)

fig, ax = plt.subplots()
ax.plot(t, vSens)
ax.plot(t, vAsps)
ax.plot(t, vLags)
ax.plot(t, vNorms)

plt.show()
