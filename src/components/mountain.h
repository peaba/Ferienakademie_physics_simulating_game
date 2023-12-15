#pragma once

#include "vector.h"
#include <array>
#include <cmath>
#include <random>
#include <vector>

struct IndexInterval {
    std::size_t start_index;
    std::size_t end_index;
};

class Mountain {
  public:
    /**
     * Number of Vertices explicitly stored by the mountain data structure
     */
    static constexpr std::size_t NUMBER_OF_VERTICES{2048};

    /**
     * width covered by mountain generated at one point in time
     */
    static constexpr float MOUNTAIN_WIDTH{2048.};

    /**
     * distance between two points of mountain
     */
    static constexpr float SECTION_WIDTH{MOUNTAIN_WIDTH / NUMBER_OF_VERTICES};

    static constexpr std::size_t NUM_SECTIONS_PER_CHUNK = 128;

    static constexpr float CHUNK_WIDTH{NUM_SECTIONS_PER_CHUNK * SECTION_WIDTH};

    /**
     * steepness of ramp generated in prototype
     */
    static constexpr float SLOPE{0.25};

    /** value beween 0 and 1 (prefereably between 0.5 and 0.75)
     *
     */
    static constexpr float ROUGHNESS_TERRAIN{0.4};

    Mountain();

    /**
     * Generates a new Chunk and deletes oldest chunk. Updates internal data
     * structures accordingly. This (prototype) version is only extending the
     * slope
     */
    void generateNewChunk();

    /**
     * Temporary helper function, do not touch
     */
    void printTempDebugInfo();

    /**
     * Example: Your rock x-coords go from [3.1, 4.2]. You call this function
     * which returns [21,24]. This means that the relevant indices for you are
     * 21, 22 and 23 (You DON'T need 24.)
     * @param min_x The leftmost x-coord relevant to you
     * @param max_x The rightmost x-coord relevant to you
     * @return Returns start_index and end_index of the section from min_x to
     * max_x INCLUDING start_index and EXCLUDING end_index;
     */
    static IndexInterval getRelevantMountainSection(float min_x, float max_x);

    /** Returns a position from a given index. The index should previously be
     * obtained via a seperate function of the mountain.
     * @param index
     * @return Position (consisting of x- and y-coordinate)
     */
    Position getVertex(size_t index);

    /** Returns a position from a given index. The index should previously be
     * obtained via a seperate function of the mountain.
     * @param index
     * @return Position (consisting of x- and y-coordinate)
     */
    Position getVertex(int index);

    /**
     * You can access all the points currently being held in the mountain
     * datastructure via mountain.getVertice(a) with a in
     * [indexInterval.start_index, indexInterval.end_index) example: the
     * indexInterval goes from 2 to 23. Now you need to access all the points
     * with the indices from 2 to 22
     * @return
     */
    IndexInterval getIndexIntervalOfEntireMountain() const;

    /**
     * @return Returns start_index and end_index of the latest generated
     * chunk. The new chunk INCLUDES start_index and EXCLUDES the end_index. You
     * can access the points via the getVertex-function.
     *
     * Start_index starts with the first new generated vertex.
     * If you want to be precise you would have to also connect the last point
     * of the previously loaded chunk and the chunk you just got by
     * getLatestChunk. You can access that point via getVertex(start_index-1).
     * However, the points might be close enough together that this is not
     * necessary. (If this API sucks it can easily be changed)
     */
    IndexInterval getLatestChunk() const;

  private:
    std::array<Position, NUMBER_OF_VERTICES>
        landscape_fixpoints_circular_array{};
    std::size_t start_of_circular_array{0};

    /** Generating a mountain using 2D Fractal Terrain Generation as described
     * in this blogpost:
     * http://nick-aschenbach.github.io/blog/2014/07/06/2d-fractal-terrain/
     *
     * @param leftIndex startIndex of the mountain section to be roughened
     * @param rightIndex endIndex of the mountain section to be roughened. The
     * Interval INCLUDES the right index
     * @param displacement Constant defining how aggressive it should be
     * roughened
     */
    void generateTerrainRecursive(std::size_t leftIndex, std::size_t rightIndex,
                                  float displacement);

    void generateSlope();

    void interpolate(std::size_t leftIndex, std::size_t rightIndex);
};
