# **CSE-322 `Computer Networks Sessional`**

 - # **`Socket Programming`**
    - [Code](Offline%20(Socket%20Programming)/SocketProgramming)

 - # **`Packet Tracer`**
   - [Packet Tracer File](Online%20(Packet%20Tracer)/1705010.pka)

 - # **`Term Project (NS3)`**
   - [Project Proposal](Term%20Project%20(NS3)/1705010_ns3_proposal.pptx)
   - [Update 1](Term%20Project%20(NS3)/1705010_ns3_update_1.pptx)
   - [Update 2](Term%20Project%20(NS3)/1705010_ns3_update_2.pptx)
   - [Experiment Graphs](Term%20Project%20(NS3)/graph)
   - [Report](Term%20Project%20(NS3)/Report.pdf)


    ## **`Task A : Wireless high-rate (mobile)`**

    ### **Topology**

    ![](Term%20Project%20(NS3)/graph/wireless-jpg/wireless-high-rate.jpg)


    ### **Simulation**

    - [Code](Term%20Project%20(NS3)/Task-A-Code/task-a-high-rate.cc)
    - [Parameter Variation Graphs](Term%20Project%20(NS3)/graph/wifi)

    ### **Sample Graph Outputs**

    ![](Term%20Project%20(NS3)/graph/wifi/node/drop.jpg)

    ![](Term%20Project%20(NS3)/graph/wifi/node/delivery.jpg)



    ## **`Task A : Wireless low-rate (static)`**


    ### **Topology**

    ![](Term%20Project%20(NS3)/graph/wireless-jpg/wireless-lrwpan.jpg)


    ### **Simulation**

    - [Code](Term%20Project%20(NS3)/Task-A-Code/task-a-low-rate.cc)
    - [Parameter Variation Graphs](Term%20Project%20(NS3)/graph/lrpwan)

    ![](Term%20Project%20(NS3)/graph/lrwpan/node/drop.jpg)

    ![](Term%20Project%20(NS3)/graph/lrwpan/node/delivery.jpg)

    # **`Task B : Implementing SRED & ESRED in ns3`**

    This repository describes the methodology to implement `Stabilized RED (SRED)`
    Algorithm in ns3. Like `RED (Random Early Detection)` SRED pre-emptively
    discards packets with a load-dependent probability when a buffer in a router in
    the Internet or an Intranet seem congested. SRED has an additional feature
    that over a wide range of load levels helps it stabilize its buffer occupation at a level independent of the number of active connections.

    We also implement an extended version of SRED where we also consider timestamp of the incoming packets in our algorithm and adjust the probability to
    overwrite accordingly. We call this `Extended Stabilized RED` or `ESRED` in short.

    ## `SRED`
    - [Implementation](Term%20Project%20(NS3)/Task-B-Code/stabilized-red-queue-disc.cc)
    - [Simulation](Term%20Project%20(NS3)/Task-B-Code/wired-red-simulation.cc)
    - Comparison with `RED`

        ![](Term%20Project%20(NS3)/graph/red-vs-sred/queue-drop.jpg)

        ![](Term%20Project%20(NS3)/graph/red-vs-sred/packet-drop.jpg)

        ![](Term%20Project%20(NS3)/graph/red-vs-sred/delivery.jpg)

    For a detialed explanation, implementation and network specification of the simulations please refer to the [report](Term%20Project%20(NS3)/Report.pdf)

    ## `ESRED`
    - [Implementation](Term%20Project%20(NS3)/Task-B-Code/es-red-queue-disc.cc)
    - [Simulation](Term%20Project%20(NS3)/Task-B-Code/wired-esred-simulation.cc)

    - Comparison with `SRED`

        ![](Term%20Project%20(NS3)/graph/sred-vs-esred/delay.jpg)

        ![](Term%20Project%20(NS3)/graph/sred-vs-esred/queue-drop.jpg)

        ![](Term%20Project%20(NS3)/graph/sred-vs-esred/packet-drop.jpg)

        ![](Term%20Project%20(NS3)/graph/sred-vs-esred/delivery.jpg)


    For a detialed explanation, implementation and network specification of the simulations please refer to the [report](Term%20Project%20(NS3)/Report.pdf)
