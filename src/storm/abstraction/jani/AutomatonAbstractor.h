#pragma once

#include "storm/storage/dd/DdType.h"

#include "storm/abstraction/jani/EdgeAbstractor.h"

#include "storm/settings/modules/AbstractionSettings.h"

#include "storm/storage/expressions/Expression.h"

#include "storm/utility/solver.h"

namespace storm {
    namespace jani {
        // Forward-declare concrete automaton class.
        class Automaton;
    }
    
    namespace abstraction {
        template<storm::dd::DdType DdType>
        struct BottomStateResult;

        namespace jani {
            template <storm::dd::DdType DdType>
            class JaniAbstractionInformation;
            
            template <storm::dd::DdType DdType, typename ValueType>
            class AutomatonAbstractor {
            public:
                /*!
                 * Constructs an abstract module from the given automaton.
                 *
                 * @param automaton The concrete automaton for which to build the abstraction.
                 * @param abstractionInformation An object holding information about the abstraction such as predicates and BDDs.
                 * @param smtSolverFactory A factory that is to be used for creating new SMT solvers.
                 * @param useDecomposition A flag indicating whether to use the decomposition during abstraction.
                 */
                AutomatonAbstractor(storm::jani::Automaton const& automaton, JaniAbstractionInformation<DdType>& abstractionInformation, std::shared_ptr<storm::utility::solver::SmtSolverFactory> const& smtSolverFactory, bool useDecomposition);
                
                AutomatonAbstractor(AutomatonAbstractor const&) = default;
                AutomatonAbstractor& operator=(AutomatonAbstractor const&) = default;
                AutomatonAbstractor(AutomatonAbstractor&&) = default;
                AutomatonAbstractor& operator=(AutomatonAbstractor&&) = default;
                
                /*!
                 * Refines the abstract automaton with the given predicates.
                 *
                 * @param predicates The new predicate indices.
                 */
                void refine(std::vector<uint_fast64_t> const& predicates);
                
                /*!
                 * Retrieves the guard of the given player 1 choice.
                 *
                 * @param player1Choice The choice of player 1.
                 * @return The guard of the player 1 choice.
                 */
                storm::expressions::Expression const& getGuard(uint64_t player1Choice) const;
                
                /*!
                 * Retrieves a mapping from variables to expressions that define their updates wrt. to the given player
                 * 1 choice and auxiliary choice.
                 */
                std::map<storm::expressions::Variable, storm::expressions::Expression> getVariableUpdates(uint64_t player1Choice, uint64_t auxiliaryChoice) const;
                
                /*!
                 * Computes the abstraction of the module wrt. to the current set of predicates.
                 *
                 * @return The abstraction of the module in the form of a BDD together with how many option variables were used.
                 */
                GameBddResult<DdType> abstract();
                
                /*!
                 * Retrieves the transitions to bottom states of this automaton.
                 *
                 * @param reachableStates A BDD representing the reachable states.
                 * @param numberOfPlayer2Variables The number of variables used to encode the choices of player 2.
                 * @return The bottom states and the necessary transitions.
                 */
                BottomStateResult<DdType> getBottomStateTransitions(storm::dd::Bdd<DdType> const& reachableStates, uint_fast64_t numberOfPlayer2Variables);
                
                /*!
                 * Retrieves an ADD that maps the encodings of edges, source/target locations and their updates to their probabilities.
                 *
                 * @return The edge decorator ADD.
                 */
                storm::dd::Add<DdType, ValueType> getEdgeDecoratorAdd() const;
                
                /*!
                 * Retrieves the abstract edges of this abstract automton.
                 *
                 * @return The abstract edges.
                 */
                std::vector<EdgeAbstractor<DdType, ValueType>> const& getEdges() const;

                /*!
                 * Retrieves the abstract edges of this abstract automaton.
                 *
                 * @return The abstract edges.
                 */
                std::vector<EdgeAbstractor<DdType, ValueType>>& getEdges();

                /*!
                 * Retrieves the number of abstract edges of this abstract automaton.
                 *
                 * @param The number of edges.
                 */
                std::size_t getNumberOfEdges() const;
                
            private:
                /*!
                 * Retrieves the abstraction information.
                 *
                 * @return The abstraction information.
                 */
                JaniAbstractionInformation<DdType> const& getAbstractionInformation() const;
                
                // A factory that can be used to create new SMT solvers.
                std::shared_ptr<storm::utility::solver::SmtSolverFactory> smtSolverFactory;
                
                // The DD-related information.
                std::reference_wrapper<JaniAbstractionInformation<DdType> const> abstractionInformation;
                
                // The abstract edge of the abstract automaton.
                std::vector<EdgeAbstractor<DdType, ValueType>> edges;
                
                // The concrete module this abstract automaton refers to.
                std::reference_wrapper<storm::jani::Automaton const> automaton;
            };
        }
    }
}