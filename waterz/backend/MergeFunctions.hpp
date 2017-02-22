#ifndef MERGE_FUNCTIONS_H__
#define MERGE_FUNCTIONS_H__

#include <algorithm>
#include <random>
#include "Operators.hpp"
#include "Histogram.hpp"

/**
 * Scores edges with min size of incident regions.
 */
template <typename SizeMapType>
class MinSize {

public:

	typedef typename SizeMapType::RegionGraphType RegionGraphType;
	typedef float                                 ScoreType;
	typedef typename RegionGraphType::NodeIdType  NodeIdType;
	typedef typename RegionGraphType::EdgeIdType  EdgeIdType;

	template <typename AffinityMapType>
	MinSize(AffinityMapType& affinities, SizeMapType& regionSizes) :
		_regionGraph(regionSizes.getRegionGraph()),
		_regionSizes(regionSizes) {}

	/**
	 * Get the score for an edge. An edge will be merged the earlier, the 
	 * smaller its score is.
	 */
	inline ScoreType operator()(EdgeIdType e) {

		return std::min(
				_regionSizes[_regionGraph.edge(e).u],
				_regionSizes[_regionGraph.edge(e).v]);
	}

	inline void notifyNodeMerge(NodeIdType from, NodeIdType to) {

		_regionSizes[to] += _regionSizes[from];
	}

	inline void notifyEdgeMerge(EdgeIdType from, EdgeIdType to) {}

private:

	const RegionGraphType& _regionGraph;
	SizeMapType            _regionSizes;
};

/**
 * Scores edges with max size of incident regions.
 */
template <typename SizeMapType>
class MaxSize {

public:

	typedef typename SizeMapType::RegionGraphType RegionGraphType;
	typedef float                                 ScoreType;
	typedef typename RegionGraphType::NodeIdType  NodeIdType;
	typedef typename RegionGraphType::EdgeIdType  EdgeIdType;

	template <typename AffinityMapType>
	MaxSize(AffinityMapType& affinities, SizeMapType& regionSizes) :
		_regionGraph(regionSizes.getRegionGraph()),
		_regionSizes(regionSizes) {}

	/**
	 * Get the score for an edge. An edge will be merged the earlier, the 
	 * smaller its score is.
	 */
	inline ScoreType operator()(EdgeIdType e) {

		return std::max(
				_regionSizes[_regionGraph.edge(e).u],
				_regionSizes[_regionGraph.edge(e).v]);
	}

	inline void notifyNodeMerge(NodeIdType from, NodeIdType to) {

		_regionSizes[to] += _regionSizes[from];
	}

	inline void notifyEdgeMerge(EdgeIdType from, EdgeIdType to) {}

private:

	const RegionGraphType& _regionGraph;
	SizeMapType            _regionSizes;
};

/**
 * Scores edges with min affinity.
 */
template <typename AffinityMapType>
class MinAffinity {

public:

	typedef typename AffinityMapType::RegionGraphType RegionGraphType;
	typedef typename AffinityMapType::ValueType       ScoreType;
	typedef typename RegionGraphType::NodeIdType      NodeIdType;
	typedef typename RegionGraphType::EdgeIdType      EdgeIdType;

	template <typename SizeMapType>
	MinAffinity(AffinityMapType& affinities, SizeMapType& regionSizes) :
		_affinities(affinities) {}

	/**
	 * Get the score for an edge. An edge will be merged the earlier, the 
	 * smaller its score is.
	 */
	inline ScoreType operator()(EdgeIdType e) {

		return _affinities[e];
	}

	void notifyNodeMerge(NodeIdType from, NodeIdType to) {}

	inline void notifyEdgeMerge(EdgeIdType from, EdgeIdType to) {

		_affinities[to] = std::min(_affinities[to], _affinities[from]);
	}

private:

	AffinityMapType& _affinities;
};

/**
 * Scores edges with a quantile affinity. The quantile is approximated by 
 * keeping track of a histogram of affinity values. This approximation is exact 
 * if the number of bins corresponds to the discretization of the affinities. 
 * The default is to use 256 bins.
 *
 * Affinities are assumed to be in [0,1].
 */
template <typename AffinityMapType, int Quantile, int Bins = 256>
class QuantileAffinity {

public:

	typedef typename AffinityMapType::RegionGraphType RegionGraphType;
	typedef typename AffinityMapType::ValueType       ScoreType;
	typedef typename RegionGraphType::NodeIdType      NodeIdType;
	typedef typename RegionGraphType::EdgeIdType      EdgeIdType;
	typedef Histogram<Bins>                           HistogramType;

	template <typename SizeMapType>
	QuantileAffinity(AffinityMapType& affinities, SizeMapType& regionSizes) :
		_affinities(affinities),
		_histograms(affinities.getRegionGraph()) {

		std::cout << "Initializing affinity histograms..." << std::endl;
		for (EdgeIdType e = 0; e < affinities.getRegionGraph().numEdges(); e++)
			_histograms[e].inc((int)(_affinities[e]*(Bins-1)));
	}

	/**
	 * Get the score for an edge. An edge will be merged the earlier, the 
	 * smaller its score is.
	 */
	ScoreType operator()(EdgeIdType e) {

		const HistogramType& histogram = _histograms[e];

		// pivot element, 1-based index
		int pivot = Quantile*histogram.sum()/100 + 1;

		int sum = 0;
		int bin = 0;
		for (bin = 0; bin < Bins; bin++) {

			sum += histogram[bin];

			if (sum >= pivot)
				break;
		}

		return (float)bin/(Bins-1);
	}

	void notifyNodeMerge(NodeIdType from, NodeIdType to) {}

	void notifyEdgeMerge(EdgeIdType from, EdgeIdType to) {

		_histograms[to] += _histograms[from];
	}

private:

	const AffinityMapType& _affinities;

	// a histogram of affinities for each edge
	typename RegionGraphType::template EdgeMap<HistogramType> _histograms;
};

/**
 * Scores edges with max affinity.
 */
template <typename AffinityMapType>
class MaxAffinity {

public:

	typedef typename AffinityMapType::RegionGraphType RegionGraphType;
	typedef typename AffinityMapType::ValueType       ScoreType;
	typedef typename RegionGraphType::NodeIdType      NodeIdType;
	typedef typename RegionGraphType::EdgeIdType      EdgeIdType;

	template <typename SizeMapType>
	MaxAffinity(AffinityMapType& affinities, SizeMapType& regionSizes) :
		_affinities(affinities) {}

	/**
	 * Get the score for an edge. An edge will be merged the earlier, the 
	 * smaller its score is.
	 */
	inline ScoreType operator()(EdgeIdType e) {

		return _affinities[e];
	}

	void notifyNodeMerge(NodeIdType from, NodeIdType to) {}

	inline void notifyEdgeMerge(EdgeIdType from, EdgeIdType to) {

		_affinities[to] = std::max(_affinities[to], _affinities[from]);
	}

private:

	AffinityMapType& _affinities;
};

/**
 * Scores edges with median affinity.
 */
template <typename AffinityMapType>
class MedianAffinity {

public:

	typedef typename AffinityMapType::RegionGraphType RegionGraphType;
	typedef typename AffinityMapType::ValueType       ScoreType;
	typedef typename RegionGraphType::NodeIdType      NodeIdType;
	typedef typename RegionGraphType::EdgeIdType      EdgeIdType;

	template <typename SizeMapType>
	MedianAffinity(AffinityMapType& affinities, SizeMapType& regionSizes) :
		_affinities(affinities),
		_affiliatedEdges(affinities.getRegionGraph()) {}

	/**
	 * Get the score for an edge. An edge will be merged the earlier, the 
	 * smaller its score is.
	 */
	ScoreType operator()(EdgeIdType e) {

		std::vector<EdgeIdType>& affiliatedEdges = _affiliatedEdges[e];

		// initial edges have their own affinity
		if (affiliatedEdges.size() == 0)
			return _affinities[e];

		// edges resulting from merges consult their affiliated edges

		auto median = affiliatedEdges.begin() + affiliatedEdges.size()/2;
		std::nth_element(
				affiliatedEdges.begin(),
				median,
				affiliatedEdges.end(),
				[this](EdgeIdType a, EdgeIdType b){

					return _affinities[a] < _affinities[b];
				}
		);

		return _affinities[*median];
	}

	void notifyNodeMerge(NodeIdType from, NodeIdType to) {}

	void notifyEdgeMerge(EdgeIdType from, EdgeIdType to) {

		if (_affiliatedEdges[from].size() == 0)
			// 'from' is an initial edge
			_affiliatedEdges[to].push_back(from);
		else
			// 'from' is a compound edge, copy its affiliated edges to the new 
			// affiliated edge list
			std::copy(
					_affiliatedEdges[from].begin(),
					_affiliatedEdges[from].end(),
					std::back_inserter(_affiliatedEdges[to]));

		// clear affiliated edges of merged region edges -- they are not 
		// needed anymore
		_affiliatedEdges[from].clear();
	}

private:

	const AffinityMapType& _affinities;

	// for every new edge between regions u and v, the edges of the initial RAG 
	// between any child of u and any child of v
	//
	// initial edges will have this empty
	typename RegionGraphType::template EdgeMap<std::vector<EdgeIdType>> _affiliatedEdges;
};

/**
 * Scores edges with a random number between 0 and 1.
 */
template <typename RegionGraphType>
class Random {

public:

	typedef float                                ScoreType;
	typedef typename RegionGraphType::NodeIdType NodeIdType;
	typedef typename RegionGraphType::EdgeIdType EdgeIdType;

	template <typename AffinityMapType, typename SizeMapType>
	Random(AffinityMapType&, SizeMapType&) {}

	/**
	 * Get the score for an edge. An edge will be merged the earlier, the 
	 * smaller its score is.
	 */
	inline ScoreType operator()(EdgeIdType e) {

		return ScoreType(rand())/RAND_MAX;
	}

	inline void notifyNodeMerge(NodeIdType from, NodeIdType to) {}

	inline void notifyEdgeMerge(EdgeIdType from, EdgeIdType to) {}
};

/**
 * Scores edges with a constant.
 */
template <typename RegionGraphType, int C>
class Const {

public:

	typedef float                                ScoreType;
	typedef typename RegionGraphType::NodeIdType NodeIdType;
	typedef typename RegionGraphType::EdgeIdType EdgeIdType;

	template <typename AffinityMapType, typename SizeMapType>
	Const(AffinityMapType&, SizeMapType&) {}

	/**
	 * Get the score for an edge. An edge will be merged the earlier, the 
	 * smaller its score is.
	 */
	inline ScoreType operator()(EdgeIdType e) {

		return C;
	}

	inline void notifyNodeMerge(NodeIdType from, NodeIdType to) {}

	inline void notifyEdgeMerge(EdgeIdType from, EdgeIdType to) {}
};

#endif // MERGE_FUNCTIONS_H__

