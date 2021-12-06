/*
  Functions supporting METAL TMG graph files.

  Jim Teresco, Fall 2021
  Siena College
*/

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tmggraph.h"
#include "sll.h"

// define the array that's externed in the header file
char *tmg_format_names[] = { "simple", "collapsed", "traveled" };

/*
  Helper function to convert a string from a traveled format graph
  connection hex code into a tmg_conn_travelers structure.

  Note: this function modifies the string passed as code.
*/
void tmg_fill_conn_travelers(tmg_conn_travelers *t, char *code) {

  short i;
  // remember the length of code, since we'll likely be putting some '\0'
  // char values into it
  int len = strlen(code);
  
  // convert all chars in the code string to 8-bit numbers in the 0-15 range
  // based on the hex code stored
  char *c = code;
  while (*c) {
    if (*c >= 'A') *c = *c - 'A' + 10;  // 'A' -> 10, 'B' -> 11, etc
    else *c -= '0';
    c++;
  }

  // count number of 1 bits
  t->count = 0;
  for (i=0; i<len; i++) {
    if (code[i] & 0x01) t->count++;
    if (code[i] & 0x02) t->count++;
    if (code[i] & 0x04) t->count++;
    if (code[i] & 0x08) t->count++;
  }

  // if non-zero, allocate the array
  if (t->count > 0) {
    t->numbers = (short *)malloc(t->count*sizeof(short));
    short tnum = 0;
    for (i=0; i<len; i++) {
      if (code[i] & 0x01) {
	t->numbers[tnum] = i*4;
	tnum++;
      }
      if (code[i] & 0x02) {
	t->numbers[tnum] = i*4+1;
	tnum++;
      }
      if (code[i] & 0x04) {
	t->numbers[tnum] = i*4+2;
	tnum++;
      }
      if (code[i] & 0x08) {
	t->numbers[tnum] = i*4+3;
	tnum++;
      }
    }
    for (i=0; i<t->count; i++) {
      printf("%d ", t->numbers[i]);
    }
    printf("\n");
  }
}

/*
  Helper function for printing waypoint entries by vertex number.
*/
void tmg_waypoint_print_by_index(int vnum, void *call_data) {

  tmg_vertex **v = (tmg_vertex **)call_data;
  tmg_waypoint_print(&(v[vnum]->w));
  printf(" ");
}

#define TMG_EQUAL_POINT_TOLERANCE 0.0000001

double tmg_distance_latlng(tmg_latlng *p1, tmg_latlng *p2) {

  // are they close enough or exactly the same point?
  if ((fabs(p1->lat-p2->lat) < TMG_EQUAL_POINT_TOLERANCE) &&
      (fabs(p1->lng-p2->lng) < TMG_EQUAL_POINT_TOLERANCE)) {
    return 0.0;
  }

  // coordinates in radians
  double rlat1 = M_PI * p1->lat / 180.0;
  double rlng1 = M_PI * p1->lng / 180.0;
  double rlat2 = M_PI * p2->lat / 180.0;
  double rlng2 = M_PI * p2->lng / 180.0;
  
  return acos(cos(rlat1)*cos(rlng1)*cos(rlat2)*cos(rlng2) +
		   cos(rlat1)*sin(rlng1)*cos(rlat2)*sin(rlng2) +
		   sin(rlat1)*sin(rlat2)) * TMG_EARTH_RADIUS;
}

/* helper function to add to an edgelist */
tmg_edgelist *tmg_edgelist_add(tmg_edge *edge, tmg_edgelist *next) {

  tmg_edgelist *newnode = (tmg_edgelist *)malloc(sizeof(tmg_edgelist));
  newnode->edge = edge;
  newnode->next = next;
  return newnode;
}

/*
  Load a graph from the given file, return a new graph pointer, NULL if 
  any problems are encountered on load.
*/
tmg_graph *tmg_load_graph(char *filename) {

  int retval;
  // a large buffer for reading in strings of varying length
  char buf[2000];
  
  FILE *f = fopen(filename, "r");
  if (!f) {
    fprintf(stderr,"Could not open file %s for reading\n", filename);
    return NULL;
  }
  
  tmg_graph *g = (tmg_graph *)calloc(1,sizeof(tmg_graph));
  
  // first line of the file is the TMG header
  retval = fscanf(f, "TMG %d.%d %s", &(g->major_version), &(g->minor_version),
		  buf);
  if (retval != 3) {
    fprintf(stderr, "Unknown TMG header format.\n");
    fclose(f);
    free(g);
    return NULL;
  }

  // version check
  if (g->major_version != 1 && g->major_version != 2) {
    fprintf(stderr, "Unknown TMG file version %d.%d.\n", g->major_version,
	    g->minor_version);
    fclose(f);
    free(g);
    return NULL;
  }

  if (strcmp(buf, "simple") == 0) {
    g->format = SIMPLE;
  }
  else if (strcmp(buf, "collapsed") == 0) {
    g->format = COLLAPSED;
  }
  else if (strcmp(buf, "traveled") == 0) {
    g->format = TRAVELED;
  }
  else {
    fprintf(stderr, "Unknown TMG file format specifier %s.\n", buf);
    fclose(f);
    free(g);
    return NULL;
  }

  // next line is number of waypoints and connections
  retval = fscanf(f, "%d%d", &(g->num_vertices), &(g->num_edges));
  if (retval != 2) {
    fprintf(stderr, "Could not read number of waypoints and connections from TMG file.\n");
    fclose(f);
    free(g);
    return NULL;
  }

  // traveled graphs have a third number, the traveler count
  if (g->format == TRAVELED) {
    retval = fscanf(f, "%d", &(g->num_travelers));
    if (retval != 1) {
      fprintf(stderr, "Could not read number of travelers from TMG file.\n");
      fclose(f);
      free(g);
      return NULL;
    }
  }

  // next g->num_vertices lines are waypoint specifications
  // label lat lng
  // allocate our array of these
  g->vertices = (tmg_vertex **)calloc(g->num_vertices,sizeof(tmg_vertex *));
  int vnum;
  for (vnum = 0; vnum < g->num_vertices; vnum++) {
    g->vertices[vnum] = (tmg_vertex *)malloc(sizeof(tmg_vertex));
    g->vertices[vnum]->vertex_num = vnum;
    g->vertices[vnum]->edges = NULL;
    retval = fscanf(f, "%s %lf %lf", buf,
		    &(g->vertices[vnum]->w.coords.lat),
		    &(g->vertices[vnum]->w.coords.lng));
    if (retval != 3) {
      fprintf(stderr, "Could not read waypoint %d from TMG\n", vnum);
      tmg_graph_destroy(g);
      fclose(f);
      free(g);
      return NULL;
    }
    g->vertices[vnum]->w.label = strdup(buf);
  }

  // next group of lines are the edges
  g->edges = (tmg_edge **)calloc(g->num_edges,sizeof(tmg_edge *));
  int ednum;
  int v1, v2;
  
  for (ednum = 0; ednum < g->num_edges; ednum++) {
    g->edges[ednum] = (tmg_edge *)calloc(1,sizeof(tmg_edge));
    // all edge lines have two vertex numbers and a label to start
    retval = fscanf(f, "%d %d %s", &v1, &v2, buf);
    if (retval != 3) {
      fprintf(stderr, "Could not read edge %d from TMG\n", ednum);
      tmg_graph_destroy(g);
      fclose(f);
      free(g);
      return NULL;
    }
    // populate the fields we have so far
    g->edges[ednum]->end1 = g->vertices[v1];
    g->edges[ednum]->end2 = g->vertices[v2];
    g->edges[ednum]->conn.end1 = &(g->vertices[v1]->w);
    g->edges[ednum]->conn.end2 = &(g->vertices[v2]->w);
    g->edges[ednum]->conn.routes = strdup(buf);

    // add to edge lists
    g->vertices[v1]->edges =
      tmg_edgelist_add(g->edges[ednum], g->vertices[v1]->edges);
    g->vertices[v2]->edges =
      tmg_edgelist_add(g->edges[ednum], g->vertices[v2]->edges);
    // traveled format graphs will next have the string representing a
    // hex number representing a bit field of who has traveled this
    // segment
    if (g->format == TRAVELED) {
      retval = fscanf(f, "%s", buf);
      tmg_fill_conn_travelers(&(g->edges[ednum]->conn.trav), buf);
    }

    // next any remaining text on the line will be lat/lng pairs for
    // the shaping points along this edge, but only for collapsed and
    // traveled format graphs
    // this section will also complete the connection length field
    if (g->format != SIMPLE) {

      // get started on length computation
      g->edges[ednum]->conn.length_in_miles = 0.0;
      tmg_latlng *prev_point = &(g->edges[ednum]->conn.end1->coords);
      
      // read the rest of the line, which, if not empty, will start
      // with a space and end with a \n
      fgets(buf, 2000, f);
      if (strlen(buf) > 1) {
	// any shaping points will be two space-separated floating
	// point values, so get a pointer that skips over the leading
	// space
	char *nextpiece = buf+1;
	nextpiece[strlen(nextpiece)-1] = '\0';
	// count the number of decimal points, which is double the
	// number of shaping points
	int count = 0;
	char *c = nextpiece;
	while (*c) {
	  if (*c == '.') count++;
	  c++;
	}
	// allocate our array of latlng structures
	g->edges[ednum]->conn.num_shaping_points = count/2;
	g->edges[ednum]->conn.shaping_points =
	  (tmg_latlng *)malloc(g->edges[ednum]->conn.num_shaping_points*sizeof(tmg_latlng));
	// now read them in
	int i;
	for (i=0; i<g->edges[ednum]->conn.num_shaping_points; i++) {
	  sscanf(nextpiece, "%lf %lf",
		 &(g->edges[ednum]->conn.shaping_points[i].lat),
		 &(g->edges[ednum]->conn.shaping_points[i].lng));
	  // add the distance from the previous to this point
	  g->edges[ednum]->conn.length_in_miles +=
	    tmg_distance_latlng(prev_point, &(g->edges[ednum]->conn.shaping_points[i]));
	  prev_point = &(g->edges[ednum]->conn.shaping_points[i]);
	  // advance over two spaces so nextpiece will point at the next
	  // pair of numbers (or the end of the string)
	  strsep(&nextpiece, " ");
	  strsep(&nextpiece, " ");
	}
      }
      // add in last distance (or all, if there were no shaping points)
      g->edges[ednum]->conn.length_in_miles +=
	tmg_distance_latlng(prev_point, &(g->edges[ednum]->conn.end1->coords));
      
    }
    else {
      // simple format, just need to compute the edge length from the
      // two latlng endpoints
      g->edges[ednum]->conn.length_in_miles =
	tmg_distance_latlng(&(g->edges[ednum]->end1->w.coords),
			    &(g->edges[ednum]->end2->w.coords));
    }
  }

  // traveled format graphs then have the list of traveler names
  if (g->format == TRAVELED) {
    g->traveler_list = (char **)malloc(g->num_travelers*sizeof(char *));
    int tnum;
    for (tnum = 0; tnum < g->num_travelers; tnum++) {
      fscanf(f, "%s", buf);
      g->traveler_list[tnum] = strdup(buf);
    }
  }
  
  fclose(f);
  return g;
}

/*
  Destroy a tmg_graph, freeing all memory.
*/
void tmg_graph_destroy(tmg_graph *g) {

  int i;
  // if the vertices array is allocated, free it
  if (g->vertices) {
    for (i=0; i<g->num_vertices; i++) {
      if (g->vertices[i]) {
	tmg_edgelist *list = g->vertices[i]->edges;
	while (list) {
	  tmg_edgelist *rmme = list;
	  list = list->next;
	  free(rmme);
	}
	free(g->vertices[i]);
      }
    }
    free(g->vertices);
  }
  
  // if the edges array is allocated, free it
  if (g->edges) {
    for (i=0; i<g->num_edges; i++) {
      if (g->edges[i]) {
	if (g->edges[i]->conn.routes) {
	  free(g->edges[i]->conn.routes);
	}
	if (g->edges[i]->conn.trav.numbers) {
	  free(g->edges[i]->conn.trav.numbers);
	}
	if (g->edges[i]->conn.shaping_points) {
	  free(g->edges[i]->conn.shaping_points);
	}
	free(g->edges[i]);
      }
    }
    free(g->edges);
  }
  
  // if a traveler list is allocated, free it
  if (g->traveler_list) {
    for (i=0; i<g->num_travelers; i++) {
      if (g->traveler_list[i]) {
	free(g->traveler_list[i]);
      }
    }
    free(g->traveler_list);
  }

  // free the tmg_graph itself
  free(g);
}

/*
  print a summary of the stats for the given graph to the FILE * 
  (can be stdout)
*/
void tmg_graph_print_stats(tmg_graph *g, FILE *fp) {

  
  fprintf(fp, "TMG version %d.%d %s format, %d vertices, %d edges\n",
	 g->major_version, g->minor_version, tmg_format_names[g->format],
	 g->num_vertices, g->num_edges);

  // find extreme waypoints, and shortest/longest labels, highest degree
  // store indices into vertex array
  int north = 0;
  int south = 0;
  int east = 0;
  int west = 0;
  int first = 0;
  int last = 0;
  sll *shortest = create_sll();
  sll_add_to_head(shortest, 0);
  sll *longest = create_sll();
  sll_add_to_head(longest, 0);
  int shortest_len = strlen(g->vertices[0]->w.label);
  int longest_len = shortest_len;
  
  int vnum;
  for (vnum = 1; vnum < g->num_vertices; vnum++) {
    // extreme coordinates
    if (g->vertices[vnum]->w.coords.lat > g->vertices[north]->w.coords.lat) {
      north = vnum;
    }
    if (g->vertices[vnum]->w.coords.lat < g->vertices[south]->w.coords.lat) {
      south = vnum;
    }
    if (g->vertices[vnum]->w.coords.lng < g->vertices[east]->w.coords.lng) {
      east = vnum;
    }
    if (g->vertices[vnum]->w.coords.lng > g->vertices[west]->w.coords.lng) {
      west = vnum;
    }

    // alphabetical
    if (strcmp(g->vertices[vnum]->w.label, g->vertices[first]->w.label) < 0) {
      first = vnum;
    }
    if (strcmp(g->vertices[vnum]->w.label, g->vertices[last]->w.label) > 0) {
      last = vnum;
    }

    // shortest and longest labels
    int len = strlen(g->vertices[vnum]->w.label);
    if (len < shortest_len) {
      shortest_len = len;
      sll_clear(shortest);
      sll_add_to_head(shortest, vnum);
    }
    else if (len == shortest_len) {
      sll_add_to_head(shortest, vnum);
    }
    if (len > longest_len) {
      longest_len = len;
      sll_clear(longest);
      sll_add_to_head(longest, vnum);
    }
    else if (len == longest_len) {
      sll_add_to_head(longest, vnum);
    }

  }

  printf("Northernmost waypoint: #%d ", north);
  tmg_waypoint_print(&(g->vertices[north]->w));
  printf("\n");
  printf("Southernmost waypoint: #%d ", south);
  tmg_waypoint_print(&(g->vertices[south]->w));
  printf("\n");
  printf("Easternmost waypoint: #%d ", east);
  tmg_waypoint_print(&(g->vertices[east]->w));
  printf("\n");
  printf("Westernmost waypoint: #%d ", west);
  tmg_waypoint_print(&(g->vertices[west]->w));
  printf("\n");
  printf("First alphabetical waypoint: #%d ", first);
  tmg_waypoint_print(&(g->vertices[first]->w));
  printf("\n");
  printf("Last alphabetical waypoint: #%d ", last);
  tmg_waypoint_print(&(g->vertices[last]->w));
  printf("\n");
  printf("Shortest waypoint labels: (len %d)\n", shortest_len);
  sll_visit_all(shortest, tmg_waypoint_print_by_index, g->vertices);
  printf("\n");
  printf("Longest waypoint labels: (len %d)\n", longest_len);
  sll_visit_all(longest, tmg_waypoint_print_by_index, g->vertices);
  printf("\n");
}

/*
  Print a waypoint in a nice format
*/
void tmg_waypoint_print(tmg_waypoint *w) {

  printf("%s (%.6f,%.6f)", w->label, w->coords.lat, w->coords.lng);
}
