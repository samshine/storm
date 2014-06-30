#ifndef STORM_PARSER_SPARSESTATEREWARDPARSER_H_
#define STORM_PARSER_SPARSESTATEREWARDPARSER_H_

#include <cstdint>
#include <vector>
#include <string>

namespace storm {
	namespace parser {

		/*!
		 * A class providing the functionality to parse a the state rewards of a model.
		 */
		class SparseStateRewardParser {
		public:

			/*!
			 *	Reads a state reward file and puts the result in a state reward vector.
			 *
			 *	@param stateCount The number of states.
			 *	@param filename The path and name of the state reward file.
			 *	@return The created state reward vector.
			 */
			static std::vector<double> parseSparseStateReward(uint_fast64_t stateCount, std::string const &filename);

		};

	} // namespace parser
} // namespace storm

#endif /* STORM_PARSER_SPARSESTATEREWARDPARSER_H_ */
