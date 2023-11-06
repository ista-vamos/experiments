#!/usr/bin/python3

import sys
import csv
import matplotlib.pyplot as plt
from matplotlib.backends.backend_pdf import PdfPages
from statistics import mean
import pandas as pd
import seaborn as sns
from os.path import basename

# source_wait_cyc, arbiter_buffer_size, source_waited,
# processed, dropped, dropped_times, delay

datait = pd.read_csv(sys.argv[1],header=None)
datait.columns = ["events","buffer_size",
                  "tau","delta","wayland-motion-events",
                  "libinput-motion-events","wayland-motions-checked",
                  "segments", "motions-ok", "motions-nok", "holes"]

datait["errors"] = 100*datait["motions-nok"]/(datait["motions-ok"] + datait["motions-nok"])
datait = datait[datait["buffer_size"] == 3]
print(datait)

############################################################3
f, ax = plt.subplots(figsize=(8, 3))
#ax.set(xscale="symlog")

plot = sns.lineplot(data=datait[datait["events"] == "events2.txt"],
                    x="delta", y="errors",
                    #style="buffer_size",
                    hue="tau",
                    ax=ax, palette="deep",
                    markers=True, dashes=False,
                    errorbar=None)
plot.legend(loc='upper right', ncol=1, title="τ [ms]")
# for line in plot.lines:
#     line.set_linestyle("--")
ax.set(xlabel='δ [pixels]', ylabel='Reported errors (%)',
      #title="% of Processed Events vs Buffer Size w/ Diff. Waiting Cycles"
       )
fig = plot.get_figure()
fig.savefig(f"wayland_plot.pdf", bbox_inches='tight', dpi=300)
fig.show()

