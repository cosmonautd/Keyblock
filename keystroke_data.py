import json
import numpy as np
import matplotlib.mlab as mlab
import matplotlib.pyplot as plt

with open("calot15b.json", "r") as f:

    keystroke_data = json.loads(f.read())

    x = np.array([v for v in keystroke_data["dd"] if v < 50])
    mu = np.mean(x)
    sigma = np.std(x)

    num_bins = 15

    fig, ax = plt.subplots()

    # the histogram of the data
    n, bins, patches = ax.hist(x, num_bins, normed=1)

    # add a 'best fit' line
    y = mlab.normpdf(bins, mu, sigma)
    ax.plot(bins, y, '--')
    ax.set_xlabel('Time (ms)')
    ax.set_ylabel('Probability density')
    ax.set_title(r'Keystroke latency: $\mu=%.2f$, $\sigma=%.2f$' % (mu, sigma))

    # Tweak spacing to prevent clipping of ylabel
    fig.tight_layout()
    plt.show()