# ROS IMC Broker

ROS IMC Broker is a [ROS](http://www.ros.org/) package that enables
interoperability between ROS nodes and [IMC](https://github.com/LSTS/imc)
capable systems.



Using It
=========
The package can be used without docker and without having DUNE installed on the system level.

* Build dune and generate a package
```
cd <your-dune-build-folder>
make package
```

* Set the environment variable `DUNE_ROOT` to the location of the generated package. This is typically found under  `_CPack_Packages/Linux/TBZ2`
```
export DUNE_ROOT=<your-dune-build-folder>/_CPack_Packages/Linux/TBZ2/dune-*/
```
* You can also set this environment variable in your `.bashrc` file for convenience.
* Your catkin workspace can now be build normally.

Trying It
=========

If you have Docker installed in your system you can navigate to the folder
"docker" and use the script docker.bash to create a docker container with
all the required software packages (build tools, ROS Indigo, DUNE, etc).

* To build the docker container navigate to the "docker" folder and issue
  the following command on a terminal:

```
bash docker.bash build
```

* Once the container is built you can open a terminal inside it by issuing
  the following command on a terminal:

```
bash docker.bash
```

This command will mount the folder "workspace" inside the container
instance. This way you can change the source code outside the container
and these changes will be reflected inside the container instance.

* To compile the broker issue the following command inside the running container:

```
catkin_make
```

This will compile the broker and some example nodes.

* To launch the follow reference example issue the following command
  inside the running container:

```
bash start.bash
```

This command will start one instance of the DUNE's LAUV simulator, the ROS
IMC broker, and the Follow Reference Controller node. This simple
controller uses DUNE's high-level control layer to move the vehicle around
a square.
