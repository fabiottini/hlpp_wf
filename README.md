# Authors

* Fabio Lucattini 

# Thesis

* https://github.com/fabiottini/hlpp_wf/blob/master/thesis/thesis_lucattini_fabio.pdf

# Related article

* http://pages.di.unipi.it/mencagli/publications/preprint-jpdc-2017.pdf
* https://www.sciencedirect.com/science/article/pii/S0743731517302976

# Description

In this work we analyze the different types of sliding windows and we classify them in order to understand how to exploit parallelism. Our model is based on the parallel execution of different windows among different in order units, because windows are usually executed independently with each other. Different types of windows have different effect on parallelism, so our implementations must be properly re-adapted to each case.
An example of classification is based on how each data item is assigned to the windows. There are windows where, at the arrival of an input item, it is immediately possible to define the complete list of windows that contain that item. We can distribute distinct windows among computing entities that elaborate them in parallel. So in presence of this characteristic, we are able to schedule each data item only to all the entities that will be responsible for executing a window that contains that item. For others window models the previous property is not satisfied: when the system receives a new item, it is not possible to define the complete list of windows that will contain that item because this depends on the temporal properties of the following items that will be received in the future. In that case we cannot evaluate a-priori the mapping between items to windows.
In this work we study a parallel pattern for computations on sliding win- dows, i.e. the windows farming. It represents an implementation of a traditional farm pattern in the window streaming context. The basic idea is to exploit parallelism by computing different windows in parallel.
We have implemented this pattern with two different approaches: the first model (the active workers model) is based on the idea to completely parallelize the queries on sliding windows: that is management of the windows in terms of insertion and expiring of items, and the processing of the window data. In this case we need to know a-priori the mapping between items to windows, and we can use a scheduler entity that is in charge of multicasting each item to all the parallel entities (workers) that will be responsible for processing a window including that data item. This approach is suitable only for windows having this characteristic, so although it is very interesting because it enables the parallelization of all the windowing phases, it has some drawbacks such as it is not applicable to all the window types.
The other approach we implemented (agnostic workers model ) is more “tra- ditional”: in this case the parallel entities (workers) are completely “anony- mous” and they are responsible only for the windows phase execution. The windows management, in terms of window update and expiring actions, is cen- tralized in a scheduler entity (emitter). This approach has some advantages in terms of load-balancing and it can be used for all the window types. However, it needs that the window implementation is designed to be used concurrently by multiple workers.

# Results

In the work we have developed some benchmarks with different window types and we have evaluated the two implementation of the pattern with the Fast-Flow framework. It is a C++ framework for streaming programming devel- oped by the Parallel Programming Research Group at the Computer Science Department of the University of Pisa.
For the window types that can be processed with both approaches we have analyzed and compared the two alternative approaches to discover which is eventually the best implementation with respect to the windowing configura- tion parameters. As expected, the two models have similar behaviors, even if the active one has an higher bandwidth due to lower internal overhead. The agnostic model remains, however, an interesting solution since it is able to achieve better load balancing and it is applicable to all the window types.

# How to use

