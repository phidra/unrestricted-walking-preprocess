#include <osmium/io/any_input.hpp>
#include <osmium/visitor.hpp>
#include <osmium/index/map/sparse_mmap_array.hpp>
#include <osmium/handler/node_locations_for_ways.hpp>
#include <osmium/geom/haversine.hpp>

#include "Graph/osmparsing.h"
#include "Graph/graph.h"

using namespace std;

namespace my::preprocess {

float geometry_length_in_meters(Polyline const& geometry) {
    // precondition = polyline has at least 2 points
    Polyline::const_iterator first = geometry.begin();
    Polyline::const_iterator second = first + 1;

    float total_length = 0;
    while (second != geometry.end()) {
        total_length += osmium::geom::haversine::distance(*first, *second);
        ++first;
        ++second;
    }
    return total_length;
}

static void add_edge(vector<Edge>& edges,
                     osmium::object_id_type node_from,
                     osmium::object_id_type node_to,
                     Polyline&& geometry,
                     float walkspeed_m_per_s) {
    float length_m = geometry_length_in_meters(geometry);
    float weight = length_m / walkspeed_m_per_s;
    edges.emplace_back(node_from, node_to, std::move(geometry), length_m, weight);
}

std::vector<Edge> build_graph(std::map<WayId, std::vector<LocatedNode> > const& way_to_nodes,
                              std::map<NodeOsmId, int> const& number_of_node_usage,
                              float walkspeed_km_per_h) {
    vector<Edge> edges;
    float walkspeed_m_per_s = walkspeed_km_per_h / 3.6;

    // precondition = each way has at least 2 nodes

    // L'objectif de ce code est de splitter en plusieurs edges les ways OSM qui sont intersectées en leur milieu
    // par d'autres ways. On utilise pour cela :
    //      way_to_nodes qui permet à partir d'une way de retrouver ses nodes
    //      number_of_node_usage qui permet à partir d'un node de savoir combien de ways l'utilisent
    // Illustration de situation (au 19 avril 2021) où ce split est nécessaire :
    //      - la Rue de Gabian (way OSM d'id 158189827) est intersectée en son milieu par la Rue de l'Industrie (way OSM
    //      d'id 446530366) :
    //              https://www.openstreetmap.org/way/158189827
    //              https://www.openstreetmap.org/way/446530366
    //      - si on se contente de créer un edge entre les noeuds extrêmes de la Rue de Gabian, le graphe résultant
    //      n'aura pas de lien entre les deux rues.
    //      - on splitte donc la Rue de Gabian en deux edges, ayant comme noeud commun l'extrêmité de la Rue de
    //      l'Industrie (node OSM d'id 2825675780) :
    //              https://www.openstreetmap.org/node/2825675780

    for (auto ite : way_to_nodes) {
        auto nodes = ite.second;

        auto first_node = nodes.begin();
        auto last_node = (nodes.end() - 1);
        while (first_node != last_node) {
            auto second_node = (first_node + 1);
            Polyline geometry{first_node->second};

            // note : pour ne pas laisser de côté les impasses, il faut obligatoirement ajouter le premier et dernier
            // node, même s'ils ont un compteur à 1

            // skipping all nodes that only belong to this way :
            while (second_node != nodes.end() && number_of_node_usage.at(second_node->first) < 2) {
                geometry.push_back(second_node->second);
                ++second_node;
            }

            // à ce stade, second_node pointe vers le premier node (après first_node) qui a un compteur >= 2
            // (ou bien vers end() s'il n'y en avait pas, ce qui veut dire que la way était une impasse se finissant sur
            // le dernier noeud)

            // si second_node est à end, c'est que le dernier node de la way n'avait pas un compteur >= 2
            // Dit autrement : la way était une impasse, se terminant sur second_node.
            // Dans ce cas, on ajoute l'edge, et on a fini pour cette way :
            if (second_node == nodes.end()) {
                add_edge(edges, first_node->first, (second_node - 1)->first, std::move(geometry), walkspeed_m_per_s);
                break;
            }

            // cas général : on ajoute le subedge, et on continue d'itérer sur la way :
            geometry.push_back(second_node->second);
            add_edge(edges, first_node->first, second_node->first, std::move(geometry), walkspeed_m_per_s);
            first_node = second_node;

            // NOTE : quoi qu'il arrive, on aura au moins un edge ajouté contenant le premier node, et un edge ajouté
            // contenant le dernier node (qui pourra ou non être le même).
        }
    }

    return edges;
}

vector<Edge> osm_to_graph(string osmfile, BgPolygon polygon, float walkspeed_km_per_h) {
    // osmium warmup :
    auto interesting_types = osmium::osm_entity_bits::node | osmium::osm_entity_bits::way;
    using Index = osmium::index::map::SparseMmapArray<osmium::unsigned_object_id_type, osmium::Location>;
    Index index;
    osmium::handler::NodeLocationsForWays<Index> location_handler{index};
    FillingHandler handler{polygon};
    osmium::io::Reader reader{osmfile, interesting_types};

    // parse osmfile + fill-in data structures :
    osmium::apply(reader, location_handler, handler);
    reader.close();

    // build graph edges :
    auto edges = build_graph(handler.way_to_nodes, handler.node_use_counter, walkspeed_km_per_h);
    return edges;
}

}  // namespace my::preprocess
