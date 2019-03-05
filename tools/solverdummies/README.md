# Solverdummies

The `solverdummies` are minimal working examples for using preCICE with different languages. Please refer to the corresponding subfolders for information on compilation and usage.

# Combining different solverdummies

preCICE allows to couple codes written in different programming languages. The solverdummies can be used to demonstrate this feature. First, you have to build the solverdummies you want to use. Then run the following commands from this folder; each command in one terminal:

```
python3 python/solverdummy.py precice-config.xml SolverOne MeshOne
./cpp/solverdummy precice-config.xml SolverTwo MeshTwo
```

This combines the python3 solverdummy with the cpp solverdummy. Note that both solverdummies use the same `precice-config.xml`. Feel free to experiment with other combinations of solverdummies or couple your own solver with one of the solverdummies.
