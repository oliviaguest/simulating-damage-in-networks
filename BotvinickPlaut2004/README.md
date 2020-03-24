# Botvinick and Plaut (2004)
This is an implementation of the Botvinick and Plaut (2004) SRN model of routine
action selection and its disorders:
> Botvinick, M., & Plaut, D. C. (2004). Doing Without Schema Hierarchies: A
> Recurrent Connectionist Approach to Normal and Impaired Routine Sequential
> Action. *Psychological Review*, 111(2), 395–429.
> https://doi.org/10.1037/0033-295X.111.2.395

Please refer to our article (especially the Supplementary Material) for more information:
> Guest, O., Caso A. & Cooper, R. P. (2020). On Simulating Neural Damage in
Connectionist Networks.


## Installation
To run the model it first needs to be compiled (once) using
[gcc](https://gcc.gnu.org):
```bash
make all
```

## Run
Then, run the command below to launch the GUI:
```bash
./xbp
```

## GUI
To explore the model's behaviour it first needs to be trained. Open the "Train" tab and train it for at least 5000 epochs (but ideally 20,000, as in the original work). See screenshots below that highlight in red what to take notice of and where to click.

![Buttons highlighted in GUI](./img/bp0.png)

![After training](./img/bp1.png)

 Then explore the model's response to damage by, for example, switching to the "BP Analyses" tab and selecting one of those analyses. In most cases, these analyses takes the currently trained network and allows exploration of its behaviour according to one of the analyses from the original (i.e., 2004) publication, or with alternative forms of damage. In some cases, however, the code uses saved weights to generate lesion results. In these cases (e.g., for survival plots), the code loads 12 different sets of saved weights, runs the specified analysis on each of the 12 networks with those weights, and then presents average results.

![Explore behaviour](./img/bp2.png)

The implementation includes three sets of analysis tabs, each with separate sub-analyses. One set of analysis (BP Analyses) corresponds to those reported by Botvinick and Plaut (2004). A second set (CS Analyses) corresponds to those reported by Cooper and Shallice (2006). The final set (JB Analyses) relate to a set of exploratory analyses motivated by the work of Jeff Bowers.
