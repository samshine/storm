#include "src/modelchecker/reachability/SparseSccModelChecker.h"

#include <algorithm>

#include "src/storage/StronglyConnectedComponentDecomposition.h"
#include "src/utility/graph.h"
#include "src/exceptions/ExceptionMacros.h"

namespace storm {
    namespace modelchecker {
        namespace reachability {
            
            template<typename ValueType>
            ValueType SparseSccModelChecker<ValueType>::computeReachabilityProbability(storm::models::Dtmc<ValueType> const& dtmc, storm::storage::BitVector const& phiStates, storm::storage::BitVector const& psiStates) {
                // First, do some sanity checks to establish some required properties.
                LOG_THROW(dtmc.getInitialStates().getNumberOfSetBits() == 1, storm::exceptions::IllegalArgumentException, "Input model is required to have exactly one initial state.");
                typename FlexibleSparseMatrix<ValueType>::index_type initialStateIndex = *dtmc.getInitialStates().begin();
                
                // Then, compute the subset of states that has a probability of 0 or 1, respectively.
                std::pair<storm::storage::BitVector, storm::storage::BitVector> statesWithProbability01 = storm::utility::graph::performProb01(dtmc, phiStates, psiStates);
                storm::storage::BitVector statesWithProbability0 = statesWithProbability01.first;
                storm::storage::BitVector statesWithProbability1 = statesWithProbability01.second;
                storm::storage::BitVector maybeStates = ~(statesWithProbability0 | statesWithProbability1);

                // If the initial state is known to have either probability 0 or 1, we can directly return the result.
                if (!maybeStates.get(initialStateIndex)) {
                    return statesWithProbability0.get(initialStateIndex) ? 0 : 1;
                }
                
                // Determine the set of states that is reachable from the initial state without jumping over a target state.
                storm::storage::BitVector reachableStates = storm::utility::graph::getReachableStates(dtmc.getTransitionMatrix(), dtmc.getInitialStates(), maybeStates, statesWithProbability1);
                
                // Subtract from the maybe states the set of states that is not reachable.
                maybeStates &= reachableStates;
                
                // Otherwise, we build the submatrix that only has the transitions of the maybe states.
                storm::storage::SparseMatrix<ValueType> submatrix = dtmc.getTransitionMatrix().getSubmatrix(false, maybeStates, maybeStates);
                
                // Then, we convert the reduced matrix to a more flexible format to be able to perform state elimination more easily.
                FlexibleSparseMatrix<ValueType> flexibleMatrix = getFlexibleSparseMatrix(dtmc.getTransitionMatrix());
                
                // Create a vector for the probabilities to go to a state with probability 1 in one step.
                std::vector<ValueType> oneStepProbabilities = dtmc.getTransitionMatrix().getConstrainedRowSumVector(maybeStates, statesWithProbability1);
                for (uint_fast64_t i = 0; i < oneStepProbabilities.size(); ++i) {
                    std::cout << i << " -> " << oneStepProbabilities[i] << std::endl;
                }
                
                // Then, we recursively treat all SCCs.
                treatScc(dtmc, flexibleMatrix, oneStepProbabilities, dtmc.getInitialStates() % maybeStates, storm::storage::BitVector(maybeStates.getNumberOfSetBits(), true), submatrix, submatrix.transpose(), false, 0);
                
                // Now, we return the value for the only initial state.
                return oneStepProbabilities[initialStateIndex];
            }
            
            template<typename ValueType>
            void SparseSccModelChecker<ValueType>::treatScc(storm::models::Dtmc<ValueType> const& dtmc, FlexibleSparseMatrix<ValueType>& matrix, std::vector<ValueType>& oneStepProbabilities, storm::storage::BitVector const& entryStates, storm::storage::BitVector const& scc, storm::storage::SparseMatrix<ValueType> const& forwardTransitions, storm::storage::SparseMatrix<ValueType> const& backwardTransitions, bool eliminateEntryStates, uint_fast64_t level) {
                std::cout << "treating SCC " << scc << " at level " << level <<  " (entry: " << entryStates << ")" << std::endl;
                if (level <= 2) {
                    std::cout << "1" << std::endl;
                    // Here, we further decompose the SCC into sub-SCCs.
                    storm::storage::StronglyConnectedComponentDecomposition<ValueType> decomposition(forwardTransitions, scc & ~entryStates, true, false);
                    std::cout << decomposition << std::endl;

                    // To eliminate the remaining one-state SCCs, we need to keep track of them.
                    storm::storage::BitVector remainingStates(scc);
                    
                    std::cout << "1" << std::endl;
                    // And then recursively treat all sub-SCCs.
                    for (auto const& newScc : decomposition) {
                        // If the SCC consists of just one state, we do not explore it recursively, but rather eliminate
                        // it directly.
                        if (newScc.size() == 1) {
                            continue;
                        }
                        
                        // Rewrite SCC into bit vector and subtract it from the remaining states.
                        storm::storage::BitVector newSccAsBitVector(forwardTransitions.getRowCount(), newScc.begin(), newScc.end());
                        remainingStates &= ~newSccAsBitVector;
                        
                        // Determine the set of entry states of the SCC.
                        storm::storage::BitVector entryStates(dtmc.getNumberOfStates());
                        for (auto const& state : newScc) {
                            for (auto const& predecessor : backwardTransitions.getRow(state)) {
                                if (predecessor.getValue() > storm::utility::constantZero<ValueType>() && !newSccAsBitVector.get(predecessor.getColumn())) {
                                    entryStates.set(state);
                                }
                            }
                        }
                        
                        // Recursively descend in SCC-hierarchy.
                        treatScc(dtmc, matrix, oneStepProbabilities, entryStates, newSccAsBitVector, forwardTransitions, backwardTransitions, true, level + 1);
                    }
                    
                    std::cout << "1" << std::endl;

                    // If we are not supposed to eliminate the entry states, we need to take them out of the set of
                    // remaining states.
                    if (!eliminateEntryStates) {
                        remainingStates &= ~entryStates;
                    }
                    
                    // Now that we eliminated all non-trivial sub-SCCs, we need to take care of trivial sub-SCCs.
                    // Therefore, we need to eliminate all states.
                    for (auto const& state : remainingStates) {
                        eliminateState(matrix, oneStepProbabilities, state, backwardTransitions);
                    }
                } else {
                    // In this case, we perform simple state elimination in the current SCC.
                    storm::storage::BitVector remainingStates(scc);

                    // If we are not supposed to eliminate the entry states, we need to take them out of the set of
                    // remaining states.
                    if (!eliminateEntryStates) {
                        remainingStates &= ~entryStates;
                    }
                    
                    // Eliminate the remaining states.
                    for (auto const& state : remainingStates) {
                        eliminateState(matrix, oneStepProbabilities, state, backwardTransitions);
                    }
                }
            }
            
            template<typename ValueType>
            void SparseSccModelChecker<ValueType>::eliminateState(FlexibleSparseMatrix<ValueType>& matrix, std::vector<ValueType>& oneStepProbabilities, uint_fast64_t state, storm::storage::SparseMatrix<ValueType> const& backwardTransitions) {
                ValueType loopProbability = storm::utility::constantZero<ValueType>();

                // Start by finding loop probability.
                for (auto const& entry : matrix.getRow(state)) {
                    if (entry.getColumn() == state) {
                        loopProbability = entry.getValue();
                        break;
                    }
                }
                
                // Scale all entries in this row with (1 / (1 - loopProbability)).
                loopProbability = 1 / (1 - loopProbability);
                for (auto& entry : matrix.getRow(state)) {
                    entry.setValue(entry.getValue() * loopProbability);
                }
                oneStepProbabilities[state] *= loopProbability;
                
                // Now connect the predecessors of the state to eliminate with its successors.
                std::size_t newEntries = matrix.getRow(state).size();
                for (auto const& predecessorEntry : backwardTransitions.getRow(state)) {
                    // First, add all entries of the successor to the list of outgoing transitions of the predecessor.
                    typename FlexibleSparseMatrix<ValueType>::row_type row = matrix.getRow(predecessorEntry.getColumn());
                    typename FlexibleSparseMatrix<ValueType>::row_type::iterator multiplyElement = std::find_if(row.begin(), row.end(), [=](storm::storage::MatrixEntry<typename FlexibleSparseMatrix<ValueType>::index_type, typename FlexibleSparseMatrix<ValueType>::value_type> const& a) { return a.getColumn() == state; });
                    
                    ValueType multiplyFactor = storm::utility::constantOne<ValueType>();
                    if (multiplyElement != row.end()) {
                        // Remove the transition to the state that is to be eliminated.
                        multiplyElement->setValue(0);
                        multiplyFactor = multiplyElement->getValue();
                    }
                    
                    // Now scale all the entries in the current row and insert them in the transitions of the predecessor.
                    row.reserve(row.size() + newEntries);
                    std::for_each(matrix.getRow(state).begin(), matrix.getRow(state).end(), [&] (storm::storage::MatrixEntry<typename FlexibleSparseMatrix<ValueType>::index_type, typename FlexibleSparseMatrix<ValueType>::value_type> const& a) { row.emplace_back(a.getColumn(), multiplyFactor * a.getValue()); });

                    // Then sort the vector according to their column indices.
                    std::sort(row.begin(), row.end(), [](storm::storage::MatrixEntry<typename FlexibleSparseMatrix<ValueType>::index_type, typename FlexibleSparseMatrix<ValueType>::value_type> const& a, storm::storage::MatrixEntry<typename FlexibleSparseMatrix<ValueType>::index_type, typename FlexibleSparseMatrix<ValueType>::value_type> const& b){ return a.getColumn() < b.getColumn(); });
                    
                    // Now we can eliminate entries with the same column by simple addition.
                    typename FlexibleSparseMatrix<ValueType>::row_type::iterator rowIt = row.begin();
                    typename FlexibleSparseMatrix<ValueType>::row_type::iterator it = row.begin();
                    typename FlexibleSparseMatrix<ValueType>::row_type::iterator rowIte = row.end();
                    for (++it; it != rowIte; ++it) {
                        if (it->getValue() == storm::utility::constantZero<ValueType>()) {
                            continue;
                        }
                        
                        if (it->getColumn() == rowIt->getColumn()) {
                            rowIt->setValue(rowIt->getValue() + it->getValue());
                        } else {
                            ++rowIt;
                            *rowIt = *it;
                        }
                    }
                    
                    // Finally, add the probabilities to go to a target state in just one step.
                    std::cout << "prior: " << oneStepProbabilities[predecessorEntry.getColumn()] << std::endl;
                    oneStepProbabilities[predecessorEntry.getColumn()] += oneStepProbabilities[state];
                    std::cout << "updating one step prob of " << predecessorEntry.getColumn() << " to " << oneStepProbabilities[predecessorEntry.getColumn()] << std::endl;
                }
            }
            
            template<typename ValueType>
            FlexibleSparseMatrix<ValueType>::FlexibleSparseMatrix(index_type rows) : data(rows) {
                // Intentionally left empty.
            }
            
            template<typename ValueType>
            void FlexibleSparseMatrix<ValueType>::reserveInRow(index_type row, index_type numberOfElements) {
                this->data[row].reserve(numberOfElements);
            }
            
            template<typename ValueType>
            typename FlexibleSparseMatrix<ValueType>::row_type& FlexibleSparseMatrix<ValueType>::getRow(index_type index) {
                return this->data[index];
            }
            
            template<typename ValueType>
            FlexibleSparseMatrix<ValueType> SparseSccModelChecker<ValueType>::getFlexibleSparseMatrix(storm::storage::SparseMatrix<ValueType> const& matrix) {
                FlexibleSparseMatrix<ValueType> flexibleMatrix(matrix.getRowCount());
                
                for (typename FlexibleSparseMatrix<ValueType>::index_type rowIndex = 0; rowIndex < matrix.getRowCount(); ++rowIndex) {
                    typename storm::storage::SparseMatrix<ValueType>::const_rows row = matrix.getRow(rowIndex);
                    flexibleMatrix.reserveInRow(rowIndex, row.getNumberOfEntries());
                    
                    for (auto const& element : row) {
                        flexibleMatrix.getRow(rowIndex).emplace_back(element);
                    }
                }
                
                return flexibleMatrix;
            }
            
            template class FlexibleSparseMatrix<double>;
            template class SparseSccModelChecker<double>;
            
        } // namespace reachability
    } // namespace modelchecker
} // namespace storm