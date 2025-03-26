#pragma once

#include <box2d/box2d.h>
#include "SFML/Graphics/ConvexShape.hpp"
#include "SFML/System/Angle.hpp"
#include "SFML/System/Vector2.hpp"
#include "box2d/collision.h"
#include "box2d/types.h"

#include <cmath>
#include <cstddef>
#include <optional>
#include <type_traits>
#include <vector>

template <typename T>
concept arithmatic = std::is_arithmetic_v<T>;

template <arithmatic T>
struct Edge {

	sf::Vector2<T> start;
	sf::Vector2<T> end;

	[[nodiscard]] sf::Angle angle() const {
		return sf::radians( static_cast<float>( std::atan2( end.y - start.y, end.x - start.x ) ) );
	}
	bool connectsWith( const Edge<T> edge ) const {
		return end == edge.start;
	}

	Edge();
	Edge( const Edge& copy ) : start(copy.start), end(copy.end) {};
	explicit Edge( sf::Vector2<T> s, sf::Vector2<T> e ) : start(s), end(e) {}
};

template <arithmatic T>
class EdgeList {
	std::vector<Edge<T>> edges;

public:
	EdgeList() = default;

	auto front() { return edges.front(); }
	auto back() { return edges.back(); }
	auto begin() { return edges.begin(); }
	const auto begin() const { return edges.cbegin(); }
	auto end() { return edges.end(); }
	const auto end() const { return edges.cend(); }

	[[nodiscard]] bool empty() {
		return edges.empty();
	}
	void push( const Edge<T>& edge ) {
		edges.emplace_back( edge );
	}
	void pop() {
		edges.pop_back();
	}

	size_t size() const {
		return edges.size();
	}

	bool enclosed() {
		return edges.back().connectsWith( edges.front() ) && edges.size() > 1;
	}

	Edge<T>& operator[]( const size_t index ) {
		return edges[index];
	}

	std::optional<size_t> findMatch( std::vector<Edge<T>> compare ) const {
		std::vector<size_t> found;
		for( size_t i = 0; i < compare.size(); i++ ) {
			if( edges.back().connectsWith( compare[i] ) ) {
				found.push_back( i );
			}
		}
		if( found.empty() ) return std::nullopt;
		// Out of the found indicies, return the clockwise-most one
		float max = -INFINITY;
		size_t clockwise_most = 0;
		for( size_t i = 0; i < found.size(); i++ ) {
			// float f = std::fmod( (edges.back().angle() - compare[found[i]].angle()).asDegrees() + 360.f, 360.f );
			float f = (edges.back().angle() - compare[found[i]].angle()).wrapUnsigned().asDegrees();
			if( f > max ) {
				max = f;
				clockwise_most = found[i];
			}
		}
		return clockwise_most;
	}
};

template <arithmatic T>
class EdgeBuffer {

	std::vector<EdgeList<T>> shapes;
	std::vector<Edge<T>> edges;
public:
	EdgeBuffer() = default;
	~EdgeBuffer() {
		clear();
	}

	auto begin() { return edges.begin(); }
	const auto begin() const { return edges.cbegin(); }
	auto end() { return edges.end(); }
	const auto end() const { return edges.cend(); }

	void clear() {
		edges.clear();
		// shapes.clear();
	}

	/// Creates a <b>copy</b> of the passed node. You do not need to worry about scope
	void pushEdge( const Edge<T>& edge ) {
		edges.emplace_back( edge );
	}
	/// Directly constructs an \link{EdgeNode} in the internal vector.
	void emplaceEdge( sf::Vector2<T> start, sf::Vector2<T> end ) {
		edges.emplace_back( start, end );
	}
	
	/**
		Returns a vector of \link{EdgeList}s, each of which is a self enclosed shapes.
		Any edges that do not enclose shapes are not used and are left in the buffer
		Edges can only be used for one shape each, and the vector may be empty.
	*/
	std::vector<EdgeList<T>> getShapes() {
		shapes.clear();
		while( !edges.empty() ) {
			EdgeList<T>& list = shapes.emplace_back();

			list.push( edges.back() );
			edges.pop_back();

			while(!list.enclosed()) {
				auto edge = list.findMatch( edges );
				if( !edge.has_value() || edges.empty() ) {
					shapes.pop_back();
					break;
				}

				// Fix #1: Fixed issue where diagonal connections caused program hang.
				// Now correctly removes edge from buffer when adding it to shape

				// We need to test if our two edges are inline.
				if( list.back().angle() == edges[edge.value()].angle() ) {
					// Merge
					std::prev(list.end())->end = edges[edge.value()].end;
				}else {
					// Append
					list.push( edges[edge.value()] );
				}
				edges.erase( edges.begin() + edge.value() );
			}
		}

		return shapes;
	}
	
	std::vector<b2Segment> getSegments() {
		static_assert(std::is_floating_point_v<T>, "EdgeBuffer::getPolygons() is only supported for floating point types");
		std::vector<b2Segment> collisions;
		for( EdgeList<T>& shape : shapes ) {
			for( size_t i = 0; i < shape.size(); i++ ) {
				collisions.push_back(b2Segment{
					{shape[i].start.x, shape[i].start.y},
					{shape[i].end.x, shape[i].end.y}
				});
			}
		}
		return collisions;
	}
	void attachChainShapes( b2BodyId body ) {
		static_assert(std::is_floating_point_v<T>, "EdgeBuffer::getChainShapes() is only supported for floating point types");
		for( auto& shape : shapes ) {
			b2ChainDef chain = b2DefaultChainDef();
			b2Vec2 points[shape.size()];
			for( size_t i = 0; i < shape.size(); i++ ) {
				points[i] = b2Vec2{shape[i].end.x, shape[i].end.y};
			}
			chain.points = points;
			chain.count = shape.size();
			chain.isLoop = true;

			b2CreateChain( body, &chain );
		}
	}
	std::vector<sf::ConvexShape> getConvexShapes() {
		static_assert(std::is_floating_point_v<T>, "EdgeBuffer::getConvexShapes() is only supported for floating point types");
		std::vector<sf::ConvexShape> convex_shapes;
		for( EdgeList<T>& shape : shapes ) {
			sf::ConvexShape& convex_shape = convex_shapes.emplace_back();
			convex_shape.setPointCount( shape.size() );
			for( size_t i = 0; i < shape.size(); i++ ) {
				convex_shape.setPoint(i, shape[i].start );
			}
		}

		return convex_shapes;
	}
};