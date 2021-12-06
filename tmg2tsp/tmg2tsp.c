/*
  Use a METAL .tmg file to generate distance matrices to use as inputs
  to the TSP programs from Pacheco, Ch. 6.

  Jim Teresco, Fall 2021
  Siena College
*/

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "tmggraph.h"

/* compute the distance between the graph vertex at index a and the
   graph vertex at index b in tenths of a mile, rounded up to the next
   tenth.
*/
int vertex_to_vertex_distance_in_tenths(tmg_graph *g, int a, int b) {

  double distance;
  if (a == b) return 0;

  distance = tmg_distance_latlng(&(g->vertices[a]->w.coords),
				 &(g->vertices[b]->w.coords));

  distance = ceil(distance * 10);

  return (int)distance;  
}

int main(int argc, char *argv[]) {

  int num_points;
  
  if (argc != 3) {
    fprintf(stderr, "Usage: %s filename numpoints\n", argv[0]);
    exit(1);
  }

  num_points = atoi(argv[2]);
  if (num_points < 2) {
    fprintf(stderr, "Number of points must be at least 2\n");
    fprintf(stderr, "Usage: %s filename numpoints\n", argv[0]);
    exit(1);
  }

  tmg_graph *g = tmg_load_graph(argv[1]);
  if (g == NULL) {
    fprintf(stderr, "Could not create graph from file %s\n", argv[1]);
    exit(1);
  }

  // start by printing the number of points
  printf("%d\n", num_points);

  // compute the distances between all pairs of the first num_points in
  // tenths of a mile, rounded up to the next tenth (to avoid any 0's)
  for (int from = 0; from < num_points; from++) {
    for (int to = 0; to < num_points; to++) {
      printf("%d\t", vertex_to_vertex_distance_in_tenths(g, from, to));
    }
    printf("\n");
  }

  printf("\n");

  // print the places and coordinates
  for (int i = 0; i < num_points; i++) {
    tmg_waypoint_print(&(g->vertices[i]->w));
    printf("\n");
  }

  printf("\nComputed from METAL .tmg file %s\n", argv[1]);
  tmg_graph_destroy(g);

  return 0;
}
