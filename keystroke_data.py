import json
import numpy as np
import matplotlib
import matplotlib.mlab as mlab
import matplotlib.pyplot as plt

font = {'family' : 'DejaVu Sans',
        'weight' : 'normal',
        'size'   : 12}

matplotlib.rc('font', **font)

def latency_plot():

    with open("calot15b.json", "r") as f, open("attack.json") as f2:

        human_latency = json.loads(f.read())
        attack_latency = json.loads(f2.read())

        x = 1000*np.array([v for v in human_latency["dd"] if v < 10.1])
        x2 = np.array([v for v in attack_latency["dd"] if v < 10000])

        mu = np.mean(x)
        sigma = np.std(x)

        mu2 = np.mean(x2)
        sigma2 = np.std(x2)

        num_bins = 10

        fig, ax = plt.subplots()

        # the histogram of the data
        weights = np.ones_like(x2)/float(len(x2))
        n, bins, patches = ax.hist(x2, num_bins, weights=weights, normed=0, color='black',\
                                    label="Attack relative frequency")
        n, bins, patches = ax.hist(x2, num_bins, normed=1, histtype='step', cumulative=-1, \
                                    label='Attack probability', linestyle='dotted', color='black')

        weights = np.ones_like(x)/float(len(x))
        n, bins, patches = ax.hist(x, num_bins, weights=weights, normed=0, color='black', alpha=0.5,\
                                    label="Human relative frequency")
        n, bins, patches = ax.hist(x, num_bins, normed=1, histtype='step', cumulative=True, \
                                    label='Human probability', linestyle='dashed', color='black', alpha=0.5)

        ax.set_xlim([100, 10000])
        ax.set_xlabel(r'Keystroke Latency ($\mu s$)')
        ax.set_ylabel('Estimated Probabilities')
        #ax.set_title(r'Keystroke latency: $\mu_h=%.2f$us, $\sigma_h=%.2f$us, $\mu_a=%.2f$us, $\sigma_a=%.2f$us' % (mu, sigma, mu2, sigma2))

        print(mu, sigma, mu2, sigma2)

        plt.legend(loc=9)

        # Tweak spacing to prevent clipping of ylabel
        fig.tight_layout()
        plt.show()

def holdtime_plot():

    with open("calot15b.json", "r") as f:

        human_latency = json.loads(f.read())

        x = np.array([v for v in human_latency["du"] if v < 50.1])
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
        ax.set_title(r'Keystroke hold time: $\mu=%.2f$ms, $\sigma=%.2f$ms' % (mu, sigma))

        # Tweak spacing to prevent clipping of ylabel
        fig.tight_layout()
        plt.show()

latency_plot()