/*
  Structure definitions and function prototypes for C representation 
  of METAL TMG graph files.

  Jim Teresco, Fall 2021
  Siena College
*/

#ifndef _TMGGRAPH_H
#define _TMGGRAPH_H

#include <stdio.h>

// upper bound on vertex label length
#define TMG_MAX_LABEL 100
// upper bound on the number of travelers for traveled-format (depends
// on the number of TM users)
#define TMG_MAX_TRAVELERS 1000

// radius of the Earth in miles
#define TMG_EARTH_RADIUS 3963.1

// all structures will have a tmg_ prefix, and each will be typedef'd to
// have a name without the "struct" for code simplicity

typedef enum tmg_format { SIMPLE, COLLAPSED, TRAVELED } tmg_format;
extern char *tmg_format_names[];

// forward references to struct defined below
struct tmg_edge;
struct tmg_edgelist;
struct tmg_vertex;

// encapsulate a simple latitude/longitude pair
typedef struct tmg_latlng {
  double lat;
  double lng;
} tmg_latlng;

// a waypoint adds a label to a latlng
typedef struct tmg_waypoint {
  tmg_latlng coords;
  char *label;
} tmg_waypoint;

// info about the travelers on a segment
typedef struct tmg_conn_travelers {
  int count;
  short *numbers; // numbers easily fit in 16 bits at this time
} tmg_conn_travelers;

// a connection is the information attached to a graph edge
typedef struct tmg_connection {
  char *routes;  // string of routes carried
  tmg_waypoint *end1;  // waypoint endpoints of this connection
  tmg_waypoint *end2;
  tmg_conn_travelers trav;  // for traveled graphs
  tmg_latlng *shaping_points;  // array of shaping points
  int num_shaping_points;
  double length_in_miles;
  
} tmg_connection;

// a graph edge has the connection plus graph vertex pointers
typedef struct tmg_edge {
  tmg_connection conn;
  struct tmg_vertex *end1;  // vertex endpoints
  struct tmg_vertex *end2;
} tmg_edge;

// a list of graph edges to be stored with graph vertices
typedef struct tmg_edgelist {
  tmg_edge *edge;
  struct tmg_edgelist *next;
} tmg_edgelist;

// graph vertex is a waypoint plus adjacency list and a number
typedef struct tmg_vertex {
  tmg_waypoint w;
  tmg_edgelist *edges;  // edges incident on this vertex
  int vertex_num;
} tmg_vertex;

// the whole graph structure
typedef struct tmg_graph {
  int major_version;
  int minor_version;
  tmg_format format;
  int num_vertices;
  tmg_vertex **vertices;
  int num_edges;
  tmg_edge **edges;  // a single list of all edges
  int num_travelers;
  char **traveler_list;  
} tmg_graph;

// function prototypes
extern tmg_latlng *tmg_latlng_create(double, double);
extern tmg_waypoint *tmg_waypoint_create(char *, double, double);
extern void tmg_waypoint_print(tmg_waypoint *w);
extern tmg_connection *tmg_connection_create(char *label, tmg_waypoint *e1,
					     tmg_waypoint *e2,
					     char *traveler_info,
					     char *shaping_text);
extern tmg_graph *tmg_load_graph(char *filename);
extern void tmg_graph_print_stats(tmg_graph *, FILE *);
extern void tmg_graph_destroy(tmg_graph *);
extern double tmg_distance_latlng(tmg_latlng *p1, tmg_latlng *p2);

#endif  // _TMGGRAPH_H
