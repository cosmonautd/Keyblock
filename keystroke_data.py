import json
import numpy as np
import matplotlib
import matplotlib.mlab as mlab
import matplotlib.pyplot as plt

font = {'family' : 'DejaVu Sans',
        'weight' : 'normal',
        'size'   : 12}

matplotlib.rc('font', **font)

with open("calot15b.json", "r") as f:

    keystroke_data = json.loads(f.read())

    x = np.array([v for v in keystroke_data["dd"] if v < 51])
    mu = np.mean(x)
    sigma = np.std(x)

    num_bins = 12

    fig, ax = plt.subplots()

    # the histogram of the data
    weights = np.ones_like(x)/float(len(x))
    n, bins, patches = ax.hist(x, num_bins, weights=weights, normed=0, color='black', alpha=0.7)
    n, bins, patches = ax.hist(x, num_bins, normed=1, histtype='step', cumulative=True, \
                                label='Empirical', linestyle='dashed', color='black')

    ax.set_xlim([1, 50])
    ax.set_xlabel('Time (ms)')
    ax.set_ylabel('Cumulative density')
    ax.set_title(r'Keystroke latency: $\mu=%.2f$ms, $\sigma=%.2f$ms' % (mu, sigma))

    # Tweak spacing to prevent clipping of ylabel
    fig.tight_layout()
    plt.show()