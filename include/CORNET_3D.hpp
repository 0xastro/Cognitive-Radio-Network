#ifndef _CORNET_3D_HPP_
#define _CORNET_3D_HPP_

//Structures used by CORNED_3D scenario controllers for backend communication
struct num_nodes_struct
{
    int num_nodes;
};

struct feedback_struct
{
    int type;
    int node;
    float frequency;
    float bandwidth;
};

struct node_struct
{
    float frequency;
    float bandwidth;
    char team_name[200];
    int role;
    int node;
};

struct crts_signal_params
{
    int type;
    int node; 
    int mod;
    int crc;
    int fec0;
    int fec1;
    double freq;
    double bandwidth;
    double gain;
};

#endif
