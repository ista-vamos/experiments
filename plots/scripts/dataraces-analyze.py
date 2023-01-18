#!/usr/bin/env python
# coding: utf-8

import pandas as pd
import seaborn as sns
import matplotlib.pyplot as plt
from matplotlib.backends.backend_pdf import PdfPages
import os, sys
from os.path import basename

def set_labels(obj, xlabel, ylabel):
    obj.set_xlabel(xlabel, labelpad=50)
    obj.set_ylabel(ylabel, labelpad=50)
    obj.tick_params(axis='x',pad=50)
    obj.tick_params(axis='y',pad=50)

infile = sys.argv[1]
data = pd.read_csv(infile, header=0, skiprows=1,
                   names=["benchmark",
                          "tsan-races", "tsan-usertime", "tsan-systime", "tsan-time", "tsan-mem",
                          "hel-races", "hel-usertime", "hel-systime", "hel-time", "hel-mem",
                          "vamos-races", "vamos-usertime", "vamos-systime", "vamos-time", "vamos-mem",
                          "vamos-eventsnum", "vamos-drop", "vamos-holes"],
                   #dtype = {"tsan-time": float,
                   #         "hel-time" : float,
                   #         "vamos-time" : float }
                   )

# first, replace time in timeouts with 0, then we will get the limits of other
# times and set a new time for timeout values
data.loc[data["tsan-races"] == "TO", "tsan-time"] = 0.0
data.loc[data["hel-races"] == "TO", "hel-time"] = 0.0
data.loc[data["vamos-races"] == "TO", "vamos-time"] = 0.0

data["tsan-time"] = data["tsan-time"].astype(float)
data["hel-time"] = data["hel-time"].astype(float)
data["vamos-time"] = data["vamos-time"].astype(float)

lim = max(max(data["tsan-time"]),
          max(data["hel-time"]),
          max(data["vamos-time"]))

timeout = 1.3*lim
lim = timeout*1.05
assert lim > 0
print(f"lim: {lim}, timeout: {timeout}")

data.loc[data["tsan-races"] == "TO", "tsan-time"] = timeout
data.loc[data["hel-races"] == "TO", "hel-time"] = timeout
data.loc[data["vamos-races"] == "TO", "vamos-time"] = timeout

fig = plt.figure(figsize=(6,6))
fig.tight_layout()
s = plt.scatter(data["tsan-time"], data["vamos-time"], color="DarkRed", marker="x",  alpha=.5)
s = plt.scatter(data["hel-time"], data["vamos-time"], color="DarkGreen", marker="x", alpha=.5)

plt.plot([0, timeout], [timeout, timeout], color="gray", linestyle="dashed")
plt.plot([timeout, timeout], [0, timeout], color="gray", linestyle="dashed")

plt.xlim(0, lim)
plt.ylim(0, lim)
plt.xticks([x/10 for x in range(0, int(10*timeout), 2)])
plt.yticks([x/10 for x in range(0, int(10*timeout), 2)])
# plt.xscale("log")
# plt.yscale("log")
#plt.grid(ls='--')
plt.legend(["TSan vs Vamos", "Helgrind vs Vamos"], loc="upper right")
plt.title("Comparing running wall-time of Vamos vs TSan and Helgrind")
plt.ylabel("VAMOS time [s]")
plt.xlabel("Tool 2 time [s]")

plt.gca().set_aspect('equal')
#plt.subplots_adjust(wspace=0.03, hspace=0)  
plt.savefig("dataraces_times.pdf", bbox_inches='tight', dpi=300) 


# plt.clf()
# lim = max(max(data["tsan-mem"]), max(data["hel-mem"]), max(data["vamos-mem"]))

# ax = data.plot.scatter(x="tsan-mem", y="vamos-mem", c="DarkRed",  marker="x")
# ax = data.plot.scatter(x="hel-mem", y="vamos-mem", c="DarkGreen", ax = ax, marker="x",
#                        ylabel="Vamos memory [KB]", xlabel="Tool 2 memory [KB]")
# ax.set_xlim(0, lim)
# ax.set_ylim(0,lim)
# plt.legend(["TSan vs Vamos", "Helgrind vs Vamos"])
# gca = plt.gca()
# gca.set_aspect('equal')
# plt.title("Comparing memory consumption of Vamos vs TSan and Helgrind")

# plt.subplots_adjust(wspace=0.03, hspace=0)  
# plt.savefig("dataraces_memory.pdf", bbox_inches='tight', dpi=300) 


# # In[192]:
# plt.clf()
# plt.figure()

# data["vamos-has-race"] = data["vamos-races"] > 0
# data["tsan-has-race"] = data["tsan-races"] > 0
# data["hel-has-race"] = data["hel-races"] > 0

# tot = len(data)
# tp = len(data.loc[data["vamos-has-race"] & data["tsan-has-race"]]) / tot
# tn = len(data.loc[(data["vamos-has-race"] == 0) & (data["tsan-has-race"] == False)]) / tot
# fp = len(data.loc[data["vamos-has-race"] & data["tsan-has-race"] == False]) / tot
# fn = len(data.loc[data["vamos-has-race"] == False & data["tsan-has-race"]]) / tot
# verdicts = [tp, tn, fp, fn]
# labels = ["true positive", "true negative", "false positive", "false negative"]

# explode = [0, 0, 0, 0]
  
# # plotting data on chart
# pie = plt.pie(verdicts, labels=labels, 
#               explode=explode,
#               autopct='%.2f%%')
# plt.title("The correctness of verdicts (taking TSan as the base line)")


# plt.subplots_adjust(wspace=0.03, hspace=0)  
# plt.savefig("dataraces_correctenss.pdf", bbox_inches='tight', dpi=300)


# data["tsan-races-diff"] = data["vamos-races"] - data["tsan-races"]
# data["hel-races-diff"] = data["vamos-races"] - data["hel-races"]

# plt.clf()
# figs_size = (10,3)
# font_scale = 1
# sns.set(rc={'figure.figsize':figs_size}, font_scale=font_scale)
# sns.set_style("whitegrid")

# fig, ax = plt.subplots(1, 2, sharey=True,
#                       gridspec_kw={'width_ratios': [1, 1]})

# b1 = sns.boxplot(data=data["tsan-races-diff"], ax=ax[0],
#                  whiskerprops=dict(linewidth=3),
#                  flierprops=dict(marker='o', markersize=2),#    markerfacecolor='red'),
#                  capprops=dict(linewidth=3),
#                  medianprops=dict(linewidth=3))
# b2 = sns.boxplot(data=data["hel-races-diff"], ax=ax[1],
#                  whiskerprops=dict(linewidth=3),
#                  flierprops=dict(marker='o', markersize=2),#    markerfacecolor='red'),
#                  capprops=dict(linewidth=3),
#                  medianprops=dict(linewidth=3))

# b1.set_xlabel('Vamos vs TSan', labelpad=15)
# b1.set_ylabel('Δ in detected races', labelpad=15)
# b2.set_xlabel('Vamos vs Helgrind', labelpad=15)
# b2.set_ylabel("")
# plt.title("The difference in the number of found data races (bigger means more found)")
# plt.subplots_adjust(wspace=0.03, hspace=0)  
# plt.savefig("dataraces_errors.pdf", bbox_inches='tight', dpi=300)


# #dataG = data.groupby("benchmark")
# #sns.violinplot(data=dataG.mean()["vamos-has-race"])


