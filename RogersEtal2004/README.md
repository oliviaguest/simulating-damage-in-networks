# Rogers et al. (2004)

This folder contains code for exploring the effects of damage on the hub and
spoke model:
> Rogers, T. T., Lambon Ralph, M. A., Garrard, P., Bozeat, S., McClelland, J.
> L., Hodges, J. R., & Patterson, K. (2004). Structure and deterioration of
> semantic memory: a neuropsychological and computational investigation.
> *Psychological review*, 111(1), 205.
> https://doi.org/10.1037/0033-295X.111.1.205

Please refer to our article (especially the Supplementary Material) for more information:
> Guest, O., Caso A. & Cooper, R. P. (2020). On Simulating Neural Damage in
Connectionist Networks.

The original intention was that model could either learn weights for
the network or load them from a file. However (and as of March, 2017),
the implementation of the learning algorithm was
unsuccessful. Instead, we generated weights using the PDPTool
implementation of the hub and spoke model and saved these to files, so
that they can be loaded and the effects of damage easily explored /
compared.


## Installation
To run the model it first needs to be compiled (once) using [gcc](https://gcc.gnu.org):
```bash
make all
```

## Run
Then, run the command below to launch the GUI:
```bash
./xhub
```

## GUI

retrained weights for various different pattern sets and different
training regimes (e.g., varying number of training epochs and learning
rate) are available in the DataFiles folders.

To explore the properties of the network with a set of trained
weights, start up the program (xhub) and select the training patterns
and weight initialisation file from the "Configuration" tab using the
"Load..." buttons. These buttons will pop up a file browser, from
where you can navigate to a folder (e.g., a subfolder of the DataFiles
folder) and select either a pattern set (a file ending in .pat) or a
weight file (a file ending in .wgt). In general, we have generated and
saved 20 different weight files for each pattern set, corresponding to
20 differently initialised networks. Once a pattern set and weights
are loaded, the properties of the pattern set can be explored (through
the "Patterns" tab) and network's behaviour can be explored (through
the "Explore" tab). Alternatively, the "Lesion Studies" can be used to
generate lesion graphs similar to those in the paper/supplementary
materials.

The "Lesion Studies" tab has several drop-down menus. These allow:

* Selection of which graph to produce. There are only two options - a
  comparison of naming performance for animals and artifacts, or a
  built-in version of a bar-chart showing the empirical data of Lambon
  Ralph et al. (2007). The latter was implemented purely to produce a
  diagram for the published paper.

* Selection of the type of damage. Four types are available, as
  described in the associated paper.

* Selection of the weights to use. If one selects ":Fixed" then the
  currently loaded weights will be used (as determined on the
  "Configuration" tab). Alternatively, one can select one of the
  folders contained pre-trained weights (e.g., p2_1000, which contains
  weights for 20 networks, each trained on pattern set p2 for 1000
  epochs). If a folder is selected, then when a lesion study is run
  (by clicking on one of the run controls toward the top right of the
  window) then the program will read a pattern set from the selected
  folder, and then a set of weights from the same folder.

The run controls consist of four buttons:
* Reset;
* Run Next (load the next network in the selected weight folder and run a
  simulation of damage using those weights and the selected form of damage);
* Run All (loop through all 20 trained networks in the selected weight folder);
* Pause (stop after the current trained network simulation completes).

Finally, there is a print button. This doesn't really print - it saves
PDF and PNG format (for both colour and black and white) of the
current graph in the FIGURES subfolder.

So, to regenerate a figure from the paper, start up xhub, select the
"Lesion Studies" tab, select a Damage type (e.g., Perturb) and Weights
folder (e.g., p1_1000), then click the "Run All" button. The graph
should appear within 30 seconds or so (depending on your machine
speed), being redrawn after each network. Within 5 minutes you should
have the final graph (reflecting the mean behaviour of 20 trained
networks).
