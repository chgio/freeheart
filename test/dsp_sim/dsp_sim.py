import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
import matplotlib.gridspec as gridspec
import matplotlib.animation as animation
from collections import deque
from itertools import *

infile = "test/data/scope_1.csv"
df = pd.read_csv(infile, index_col=False)

t = df.time.to_numpy()
vSens = df.ch1.to_numpy() - 1.36
vAsps = df.ch2.to_numpy()

size = 960
head_size = 195
tail_size = 32

def firstOrderLag(queue, alpha):
    nqueue = queue.copy()

    for i in range(tail_size, size, 1):
        x = nqueue[i]
        y = nqueue[i-1]
        nqueue[i] = (alpha) * x + (1-alpha) * y
    
    return nqueue


def rollingAvgNorm(queue, offset):
    nqueue = queue.copy()
    avgs = [0 for i in range(size)]

    for i in range(tail_size, size, 1):
        y = 0
        
        for j in range(i-tail_size, i, 1):
            x = nqueue[j]
            y += x / tail_size
        
        avgs[i] = y
    
    for i in range(tail_size, size, 1):
        y = nqueue[i] + offset - avgs[i]
        nqueue[i] = y
    
    return nqueue


def discretise(queue):
    nqueue = queue.copy()
    
    qmin, qmax = min(nqueue), max(nqueue)
    delta = (qmax-qmin) / 7
    
    for i in range(tail_size, size, 1):
        nqueue[i] = (nqueue[i] - qmin) // delta

    return nqueue


def bpm(queue, sample_freq, rmin, rmax):
    nqueue = queue.copy()

    qmin, qmax = min(nqueue), max(nqueue)
    delta = qmax-qmin
    tmin = qmin + rmin * delta
    tmax = qmin + rmax * delta
    
    sample_periods = []
    ilast = 0
    above = False
    for i in range(tail_size, size, 1):

        if not above and nqueue[i] >= tmax:
            sample_periods.append(i - ilast)
            ilast = i
            above = True
        elif above and nqueue[i] <= tmin:
            above = False
    
    period_avg = (sum(sample_periods) / len(sample_periods)) / sample_freq
    return 60 / period_avg


def bpms(queue, sample_freq, rmin, rmax):
    tmins = [0 for i in range(size)]
    tmaxs = [0 for i in range(size)]
    bpms = [0 for i in range(size)]

    for i in range(head_size, size, 1):
        mqueue = deque(islice(queue, i-head_size, i))

        qmin, qmax = min(mqueue), max(mqueue)
        delta = (qmax-qmin)
        tmin = qmin + rmin * delta
        tmax = qmin + rmax * delta
        
        sample_periods = []
        jlast = 0
        above = False
        for j in range(0, i, 1):

            if not above and queue[j] >= tmax:
                sample_periods.append(j - jlast)
                jlast = j
                above = True
            elif above and queue[j] <= tmin:
                above = False
        
        period_avg = (sum(sample_periods) / len(sample_periods)) / sample_freq
        tmins[i] = tmin
        tmaxs[i] = tmax
        bpms[i] = 60 / period_avg
    
    return tmins, tmaxs, bpms


edge = deque(maxlen=size)

for i, v in enumerate(vAsps):
    # artificially simulate upwards drift
    edge.append(v + 0.0001*i)

fol = firstOrderLag(edge, 0.15)
ran = rollingAvgNorm(fol, 1.60)
disc = discretise(ran)
bpm = bpm(ran, 200, 0.3, 0.7)
tmins, tmaxs, bpms = bpms(ran, 200, 0.3, 0.7)


def still_view(head_size):
    fig = plt.figure(tight_layout=True)
    gs = gridspec.GridSpec(2, 2)

    ax0 = fig.add_subplot(gs[0, :])
    ax1 = fig.add_subplot(gs[1, :])
    """
    ax2 = fig.add_subplot(gs[1, 1])
    """

    #sens_line, = ax0.plot(t[:head_size], vSens[:head_size], alpha=0.40)
    #edge_line, = ax0.plot(t[:head_size], deque(islice(edge, 0, head_size)), alpha=1.0)
    #fol_line, = ax0.plot(t[:head_size], deque(islice(fol, 0, head_size)), alpha=0.40)
    ran_line, = ax0.plot(t[:head_size], deque(islice(ran, 0, head_size)), alpha=1.0)
    tmin_line, = ax0.plot(t[:head_size], tmins[:head_size], color="tab:grey", ls="-", lw=0.8, alpha=0.60)
    tmax_line, = ax0.plot(t[:head_size], tmaxs[:head_size], color="tab:grey", ls="-", lw=0.8, alpha=0.60)
    #disc_line, = ax1.plot(t[:head_size], deque(islice(disc, 0, head_size)), linestyle='', marker="o", color="tab:red")
    bpm_line, = ax1.plot(t[:head_size], deque(islice(bpms, 0, head_size)), color="tab:red")
    #bpm_text, = ax2.text(0.4, 0.4, f"{bpm:.0f}", size=75)

    ax0.set_ylim(1.50, 1.75)
    ax0.grid(ls="--", lw= 1.2, alpha=0.6)
    ax0.legend([
        #"Sensor output",
        #"ASP output",
        #"1st order lag filter",
        #"Rolling avg. normalisation",
        "DSP output",
        "Double thresholds"
    ])
    ax0.set_xlabel("time [s]")
    ax0.set_ylabel("voltage [V]")

    #ax1.set_yticks([i for i in range(8)])
    ax1.grid(ls="--", lw= 1.2, alpha=0.6)
    ax1.legend([
        "Pulse rate",
    ])
    ax1.set_xlabel("time [s]")
    ax1.set_ylabel("frequency [BPM]")

    #ax2.set_xticks([])
    #ax2.set_yticks([])
    #ax2.text(0.43, 0.8, "BPM", size=25)

    plt.show()


def scrolling_view(head_size):
    fig = plt.figure(figsize=(12.8, 7.2), tight_layout=True)
    gs = gridspec.GridSpec(2, 2)

    ax0 = fig.add_subplot(gs[0, :])
    ax1 = fig.add_subplot(gs[1, 0])
    ax2 = fig.add_subplot(gs[1, 1])

    sens_line, = ax0.plot(t[:head_size], vSens[:head_size], alpha=0.30)
    edge_line, = ax0.plot(t[:head_size], deque(islice(edge, 0, head_size)), alpha=0.30)
    fol_line, = ax0.plot(t[:head_size], deque(islice(fol, 0, head_size)))
    ran_line, = ax0.plot(t[:head_size], deque(islice(ran, 0, head_size)))
    tmin_line, = ax0.plot(t[:head_size], tmins[:head_size], color="tab:grey", ls="-", lw=0.8, alpha=0.60)
    tmax_line, = ax0.plot(t[:head_size], tmaxs[:head_size], color="tab:grey", ls="-", lw=0.8, alpha=0.60)
    disc_line, = ax1.plot(t[:head_size], deque(islice(disc, 0, head_size)), linestyle='', marker="o", color="tab:red")
    bpm_text = ax2.text(0.4, 0.4, f"{bpm:.0f}", size=75)

    ax0.set_ylim(1.5, 1.75)
    ax0.grid(ls="--", lw= 1.2, alpha=0.6)
    ax0.legend([
        "Sensor output",
        "ASP output",
        "1st order lag filter",
        "Rolling avg. normalisation",
    ], loc="upper left")

    ax1.set_yticks([i for i in range(8)])
    ax1.grid(ls="--", lw= 1.2, alpha=0.6)
    ax1.legend([
        "Discretisation output",
    ], loc="upper left")

    ax2.set_xticks([])
    ax2.set_yticks([])
    ax2.text(0.43, 0.8, "BPM", size=25)

    def animate(i):
        sens_line.set_ydata(vSens[i:i+head_size])
        edge_line.set_ydata(deque(islice(edge, i, i+head_size)))
        fol_line.set_ydata(deque(islice(fol, i, i+head_size)))
        ran_line.set_ydata(deque(islice(ran, i, i+head_size)))
        tmin_line.set_ydata(deque(islice(tmins, i, i+head_size)))
        tmax_line.set_ydata(deque(islice(tmaxs, i, i+head_size)))
        disc_line.set_ydata(deque(islice(disc, i, i+head_size)))
        bpm_text.set_text(f"{bpms[i]:.0f}")
        return [
            sens_line,
            edge_line,
            fol_line,
            ran_line,
            tmin_line,
            tmax_line,
            disc_line,
            bpm_text,
        ]

    ani = animation.FuncAnimation(fig, animate, interval=20, blit=True, save_count=60*12)

    vwriter = animation.FFMpegWriter(fps=60)
    gifwriter = animation.PillowWriter(fps=30)
    #ani.save("test/heart.mp4",writer=vwriter)
    #ani.save("test/heart.gif", writer=gifwriter)
    plt.show()


#still_view(size)
scrolling_view(head_size)
