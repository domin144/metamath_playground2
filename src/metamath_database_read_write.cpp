/* 
 * Copyright 2013, 2016 Dominik Wójt
 * 
 * This file is part of metamath_playground.
 * 
 * metamath_playground is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * metamath_playground is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with metamath_playground.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "metamath_database_read_write.h"
#include "tokenizer.h"

#include <stdexcept>
#include <utility>
#include <map>
#include <set>
#include <tuple>
#include <algorithm>

namespace metamath_playground {
/*----------------------------------------------------------------------------*/
namespace {
/*----------------------------------------------------------------------------*/
struct frame_entry
{
    enum class type_t
    {
        disjoint_variable_restriction,
        essential_hypothesis,
        floating_hypothesis
    };

    type_t type;
    int index; /* index in its category */
};
/*----------------------------------------------------------------------------*/
using frame = std::vector<frame_entry>;
/*----------------------------------------------------------------------------*/
/* Only ordinary (not extended) frames are kept in this registry. */
struct legacy_frame_registry
{
    /* Indices in this context refer to assertion's internal arrays. */
    std::vector<frame> frames;
};
/*----------------------------------------------------------------------------*/
class scope
{
private:
    std::map<std::string, int> label_to_essential_hypothesis_index;
    std::vector<floating_hypothesis> floating_hypotheses;
    std::vector<essential_hypothesis> essential_hypotheses;
    std::vector<disjoint_variable_restriction> disjoint_variable_restrictions;
    std::vector<frame_entry> spurious_frame;

public:
    scope() = default;
    scope(scope &) = default;

    void add_floating_hypothesis(floating_hypothesis &&hypothesis);
    /* Returns -1 on failure. */
    int find_essential_hypothesis_index_by_label(
            const std::string &label) const;
    void add_essential_hypothesis(essential_hypothesis &&hypothesis);
    void add_disjoint_variable_restriction(
            disjoint_variable_restriction &&restriction);

    const std::vector<essential_hypothesis> &get_essential_hypotheses() const
    {
        return essential_hypotheses;
    }

    const std::vector<floating_hypothesis> &get_floating_hypotheses() const
    {
        return floating_hypotheses;
    }

    const std::vector<disjoint_variable_restriction>
                &get_disjoint_variable_restrictions() const
    {
        return disjoint_variable_restrictions;
    }

    const frame &get_spurious_frame() const
    {
        return spurious_frame;
    }
};
/*----------------------------------------------------------------------------*/
void scope::add_floating_hypothesis(floating_hypothesis &&hypothesis)
{
    floating_hypotheses.push_back(std::move(hypothesis));
    spurious_frame.push_back(
                frame_entry{
                    frame_entry::type_t::floating_hypothesis,
                    static_cast<int>(floating_hypotheses.size() - 1)});
}
/*----------------------------------------------------------------------------*/
void scope::add_essential_hypothesis(essential_hypothesis &&hypothesis)
{
    /* TODO: Put global check for name clashes at level above. */
    if(find_essential_hypothesis_index_by_label(hypothesis.get_name()) != -1)
        throw std::runtime_error("statement name clash");

    essential_hypotheses.push_back(std::move(hypothesis));

    auto &added_hypothesis = essential_hypotheses.back();
    label_to_essential_hypothesis_index[added_hypothesis.get_name()] =
            static_cast<int>(essential_hypotheses.size() - 1);

    spurious_frame.push_back(
                frame_entry{
                    frame_entry::type_t::essential_hypothesis,
                    static_cast<int>(essential_hypotheses.size() - 1)});
}
/*----------------------------------------------------------------------------*/
void scope::add_disjoint_variable_restriction(
        disjoint_variable_restriction &&restriction)
{
    disjoint_variable_restrictions.push_back(std::move(restriction));
    spurious_frame.push_back(
                frame_entry{
                    frame_entry::type_t::disjoint_variable_restriction,
                    static_cast<int>(
                        disjoint_variable_restrictions.size() - 1)});
}
/*----------------------------------------------------------------------------*/
void read_comment(tokenizer &tokenizer0);
/*----------------------------------------------------------------------------*/
expression read_expression(
        Metamath_database &database,
        tokenizer &input_tokenizer,
        const std::string &terminating_token = "$.")
{
    expression result;
    while (input_tokenizer.peek() != terminating_token)
    {
        if (input_tokenizer.peek() == "$(")
            read_comment(input_tokenizer);
        auto symbol =
                database.find_symbol_by_label(input_tokenizer.get_token());
        if (!symbol)
            throw std::runtime_error("symbol not found");
        result.push_back(symbol);
    }
    return result;
}
/*----------------------------------------------------------------------------*/
void read_statement(
        Metamath_database &database,
        legacy_frame_registry &registry,
        scope &current_scope,
        tokenizer &input_tokenizer);
/*----------------------------------------------------------------------------*/
void read_scope(
        Metamath_database &database,
        legacy_frame_registry &registry,
        scope &parent_scope,
        tokenizer &input_tokenizer)
{
    if (input_tokenizer.get_token() != "${")
        throw std::runtime_error("scope does not start with \"${\"");

    scope current_scope(parent_scope);
    while (input_tokenizer.peek() != "$}")
        read_statement(database, registry, current_scope, input_tokenizer);
    input_tokenizer.get_token(); /* consume "$}" */
}
/*----------------------------------------------------------------------------*/
void read_variables(Metamath_database &database, tokenizer &input_tokenizer)
{
    if (input_tokenizer.get_token() != "$v")
        throw std::runtime_error("variables do not start with \"$v\"");

    while (input_tokenizer.peek() != "$.")
    {
        if (input_tokenizer.peek() == "$(")
            read_comment(input_tokenizer);
        const auto name = input_tokenizer.get_token();
        database.add_symbol(Symbol(Symbol::Type::variable, name));
    }
    input_tokenizer.get_token(); /* consume "$." */
}
/*----------------------------------------------------------------------------*/
void read_constants(Metamath_database &database, tokenizer &input_tokenizer)
{
    if (input_tokenizer.get_token() != "$c")
        throw std::runtime_error("constants do not start with \"$c\"");

    while (input_tokenizer.peek() != "$.")
    {
        if (input_tokenizer.peek() == "$(")
            read_comment(input_tokenizer);
        const auto name = input_tokenizer.get_token();
        database.add_symbol(Symbol(Symbol::Type::constant, name));
    }
    input_tokenizer.get_token(); /* consume "$." */
}
/*----------------------------------------------------------------------------*/
void read_floating_hypothesis(
        Metamath_database &database,
        scope &current_scope,
        tokenizer &input_tokenizer,
        const std::string &label)
{
    if (input_tokenizer.get_token() != "$f")
        throw std::runtime_error("variable assumption does not start with "
            "\"$f\"");

    const auto expression = read_expression(database, input_tokenizer);
    const auto &symbols = expression.get_symbols();
    if (
            symbols.size() != 2
            || symbols[0]->get_type() != Symbol::Type::constant
            || symbols[1]->get_type() != Symbol::Type::variable)
        throw std::runtime_error("invalid floating hypothesis");

    floating_hypothesis hypothesis(label, symbols[0], symbols[1]);
    current_scope.add_floating_hypothesis(std::move(hypothesis));

    input_tokenizer.get_token(); /* consume "$." */
}
/*----------------------------------------------------------------------------*/
void read_essential_hypothesis(
        Metamath_database &database,
        scope &current_scope,
        tokenizer &input_tokenizer,
        const std::string &label)
{
    if (input_tokenizer.get_token() != "$e")
        throw std::runtime_error("assumption does not start with \"$e\"");

    auto expression0 = read_expression(database, input_tokenizer);
    essential_hypothesis hypothesis(label, expression0);
    current_scope.add_essential_hypothesis(std::move(hypothesis));

    input_tokenizer.get_token(); /* consume "$." */
}
/*----------------------------------------------------------------------------*/
void collect_variables(
        const expression &expression0,
        std::set<const Symbol *> &symbols_found,
        std::vector<const Symbol *> &result)
{
    /* Algorithm is designed to preserve the order of symbols in expression. */

    for(auto symbol : expression0.get_symbols())
    {
        if(
                symbol->get_type() == Symbol::Type::variable
                && symbols_found.count(symbol) == 0)
        {
            symbols_found.insert(symbol);
            result.push_back(symbol);
        }
    }
}
/*----------------------------------------------------------------------------*/
/* The order of variables in frame is the order of first occurance in hypotheses
 * and expression. */
std::vector<const Symbol *> collect_variables(
        const std::vector<essential_hypothesis> &hypotheses,
        const expression &expression0)
{
    std::set<const Symbol *> symbols_found;
    std::vector<const Symbol *> result;

    for(auto &hypothesis : hypotheses)
        collect_variables(hypothesis.get_expression(), symbols_found, result);
    collect_variables(expression0, symbols_found, result);

    return result;
}
/*----------------------------------------------------------------------------*/
std::vector<disjoint_variable_restriction> filter_restrictions(
        const std::vector<disjoint_variable_restriction> &input_restrictions,
        const std::vector<const Symbol *> variables)
{
    std::vector<disjoint_variable_restriction> result;
    for (auto restriction : input_restrictions)
    {
        const auto disjoint_variables = restriction.get_variables();
        auto v0_iterator =
                std::find(
                    variables.begin(),
                    variables.end(),
                    disjoint_variables[0]);
        auto v1_iterator =
                std::find(
                    variables.begin(),
                    variables.end(),
                    disjoint_variables[1]);
        if (v0_iterator != variables.end() && v1_iterator != variables.end())
            result.push_back(restriction);
    }
    return result;
}
/*----------------------------------------------------------------------------*/
std::tuple<std::vector<floating_hypothesis>, frame>
fill_legacy_frame_and_floating_hypotheses(
        const frame &spurious_frame,
        const std::vector<floating_hypothesis> input_hypotheses,
        const std::vector<const Symbol *> variables)
{
    std::vector<floating_hypothesis> floating_hypotheses;
    frame legacy_frame;
    int essential_hypothesis_index = 0;
    int floating_hypothesis_index = 0;
    for(auto entry : spurious_frame)
    {
        switch (entry.type)
        {
        case frame_entry::type_t::disjoint_variable_restriction:
            break;
        case frame_entry::type_t::essential_hypothesis:
            legacy_frame.push_back(
                        frame_entry{entry.type, essential_hypothesis_index});
            ++essential_hypothesis_index;
            break;
        case frame_entry::type_t::floating_hypothesis: {
            auto &hypothesis = input_hypotheses[entry.index];
            auto iterator =
                    std::find(
                        variables.begin(),
                        variables.end(),
                        hypothesis.get_variable());
            if (iterator == variables.end())
                break;
            floating_hypotheses.push_back(hypothesis);
            legacy_frame.push_back(
                        frame_entry{entry.type, floating_hypothesis_index});
            ++floating_hypothesis_index;
            break; }
        }
    }
    return std::make_tuple(floating_hypotheses, legacy_frame);
}
/*----------------------------------------------------------------------------*/
class compressed_proof_code_extractor
{
private:
    tokenizer &tokenizer0;
    std::string buffer;

public:
    compressed_proof_code_extractor(tokenizer &tokenizer_in) :
        tokenizer0(tokenizer_in)
    { }
    int extract_number()
    {
        int n = 0;
        while (true)
        {
            char z = get_character();
            if ('A' <= z && z <= 'T')
            {
                n = n * 20 + z - 'A' + 1;
                return n;
            }
            else if ('U' <= z && z <= 'Y')
            {
                n = n * 5 + z - 'U' + 1;
            }
            else if (z == 'Z')
            {
                throw std::runtime_error(
                            "error: Z found in compressed proof when number "
                            "incomplete");
            }
            else
            {
                throw std::runtime_error(
                            "error: invalid character found in compressed "
                            "proof");
            }
        }
    }
    bool extract_reference_flag()
    {
        if (peek_character() == 'Z')
        {
            get_character();
            return true;
        }
        else
        {
            return false;
        }
    }
    bool is_end_of_proof()
    {
        return peek_character() == 0;
    }

private:
    void fill_buffer()
    {
        if (tokenizer0.peek() != "$.")
            buffer += tokenizer0.get_token();
    }
    char peek_character()
    {
        if (buffer.empty())
            fill_buffer();
        if (buffer.empty())
            return 0; /* end of compressed sequence */
        else
            return buffer.front();
    }
    char get_character()
    {
        char result = peek_character();
        if (result == 0)
            throw std::runtime_error(
                    "read got past end of compressed sequence");
        buffer.erase(buffer.begin());
        return result;
    }
};
/*----------------------------------------------------------------------------*/
//class Proof_collector : private Lazy_const_statement_visitor
//{
//public:
//    Proof_step *new_step( const Statement *statement )
//    {
//        statement->accept( *this );
//        if( !m_step )
//            throw std::runtime_error( "invalid proof step read" );
//        return m_step;
//    }
//private:
//    Proof_step *m_step = 0;
//    void operator()( const Floating_hypothesis *hypothesis ) override
//    {
//        m_step = new Floating_hypothesis_step( hypothesis );
//    }
//    void operator()( const Essential_hypothesis *hypothesis ) override
//    {
//        m_step = new Essential_hypothesis_step( hypothesis );
//    }
//    void operator()( const Theorem *theorem ) override
//    {
//        new_assertion_step( theorem );
//    }
//    void operator()( const Axiom *axiom ) override
//    {
//        new_assertion_step( axiom );
//    }
//    void new_assertion_step( const Assertion *assertion )
//    {
//        m_step = new Assertion_step( assertion );
//    }
//};
/*----------------------------------------------------------------------------*/
//void read_compressed_proof( Scoping_statement *scope, Tokenizer &tokenizer,
//    Proof &proof, const Theorem *theorem )
//{
//    proof.set_compressed( true );
//    tokenizer.get_token(); // read "("

//    std::vector<const Named_statement *> mandatory_hypotheses;
//    {
//        Frame frame( theorem );
//        collect_frame( frame );
//        std::swap( mandatory_hypotheses, frame.get_statements() );
//        mandatory_hypotheses.pop_back();
//    }
//    const int mandatory_hypotheses_count = mandatory_hypotheses.size();

//    // read referred statements
//    std::vector<const Named_statement *> referred_statements;
//    while( tokenizer.peek() != ")" )
//    {
//        auto statement = scope->get_statement_by_label( tokenizer.get_token() );
//        referred_statements.push_back( statement );
//    }
//    const int referred_statements_count = referred_statements.size();
//    tokenizer.get_token(); // read ")"

//    int referred_expressions_count = 0;
//    Compressed_proof_code_extractor extractor( tokenizer );
//    Proof_collector proof_collector;
//    while( !extractor.is_end_of_proof() )
//    {
//        int number = extractor.extract_number()-1;
//        if( number < mandatory_hypotheses_count )
//        {
//            proof.get_steps().push_back( proof_collector.new_step(
//                mandatory_hypotheses[number] ) );
//        }
//        else if( ( number -= mandatory_hypotheses_count ) <
//            referred_statements_count )
//        {
//            proof.get_steps().push_back( proof_collector.new_step(
//                referred_statements[number] ) );
//        }
//        else if( ( number -= referred_statements_count ) <
//            referred_expressions_count )
//        {
//            proof.get_steps().push_back( new Refer_step( number ) );
//        }
//        else
//        {
//            throw std::runtime_error(
//                "invalid number read in compressed proof" );
//        }
//        if( extractor.extract_reference_flag() )
//        {
//            proof.get_steps().push_back( new Add_reference_step() );
//            referred_expressions_count++;
//        }
//    }
//}
//------------------------------------------------------------------------------
proof read_uncompressed_proof(
        Metamath_database &database,
        scope &current_scope,
        tokenizer &input_tokenizer,
        legacy_frame_registry &frame_registry)
{
    std::vector<proof_step> steps;

    while (input_tokenizer.peek() != "$.")
    {
        auto name = input_tokenizer.get_token();
        auto assertion = database.find_assertion_by_label(name);
        if (name == "?")
        {
            steps.push_back(proof_step{proof_step::type_t::unknown, 0, 0});
        }
        else if (assertion != nullptr)
        {
            const int index = database.get_assertion_index(assertion);
            const int consumed_count =
                    static_cast<int>(frame_registry.frames[index].size());
            steps.push_back(
                        proof_step{
                            proof_step::type_t::assertion,
                            index,
                            consumed_count});
        }
        else if (/* essential hypothesis */ false)
        {
            /* just push with index */
        }
        else if (/* mandatory floating hypothesis */ false)
        {
            /* just push with index */
        }
        else if (/* non-mandatory floating hypothesis */ false)
        {
            /* add to proof and push with
             * index = (index in proof) + (mandatory floating hypotheses count)
             */
        }
    }
}
/*----------------------------------------------------------------------------*/
void read_assertion(
        Metamath_database &database,
        scope &current_scope,
        legacy_frame_registry &registry,
        tokenizer &input_tokenizer,
        const std::string &label)
{
    assertion::type_t type;
    if (input_tokenizer.peek() == "$a")
        type = assertion::type_t::axiom;
    else if (input_tokenizer.peek() == "$p")
        type = assertion::type_t::theorem;
    else
        throw std::runtime_error("axiom does not start with \"$a\"");
    input_tokenizer.get_token(); /* consume "$a" or "$p" */

    const std::string expression_terminator =
            type == assertion::type_t::axiom
            ? "$."
            : "$=";
    expression expression0 =
            read_expression(database, input_tokenizer, expression_terminator);
    auto essential_hypotheses = current_scope.get_essential_hypotheses();
    auto variables = collect_variables(essential_hypotheses, expression0);
    auto disjoint_variable_restrictions =
            filter_restrictions(
                current_scope.get_disjoint_variable_restrictions(),
                variables);

    std::vector<floating_hypothesis> floating_hypotheses;
    frame legacy_frame;
    std::tie(floating_hypotheses, legacy_frame) =
            fill_legacy_frame_and_floating_hypotheses(
                current_scope.get_spurious_frame(),
                current_scope.get_floating_hypotheses(),
                variables);

    registry.frames.push_back(legacy_frame);

    switch (type)
    {
    case assertion::type_t::axiom: {
        assertion new_assertion(
                    label,
                    type,
                    std::move(disjoint_variable_restrictions),
                    std::move(floating_hypotheses),
                    std::move(essential_hypotheses),
                    std::move(expression0));
        database.add_assertion(std::move(new_assertion));
        break; }
    case assertion::type_t::theorem: {
        input_tokenizer.get_token(); /* consume "$=" */

        if(input_tokenizer.peek() == "(")
        {
            throw "TODO";
    //        read_compressed_proof( scope, tokenizer, theorem->get_proof(),
    //            theorem );
        }
        else
        {
            throw "TODO";
    //        read_uncompressed_proof( scope, tokenizer, theorem->get_proof() );
        }
        assertion new_assertion(
                    label,
                    type,
                    std::move(disjoint_variable_restrictions),
                    std::move(floating_hypotheses),
                    std::move(essential_hypotheses),
                    std::move(expression0));
        database.add_assertion(std::move(new_assertion));
        break; }
    }

    input_tokenizer.get_token(); /* consume "$." */
}
//------------------------------------------------------------------------------
void read_disjoint_variable_restriction(
        Scoping_statement *scope,
        Tokenizer &tokenizer,
        const std::string &label )
{
    if( tokenizer.get_token() != "$d" )
        throw( std::runtime_error("disjoint variable restriction does not start"
            " with \"$v\"") );

    auto restriction = new Disjoint_variable_restriction();
    scope->add_statement( restriction );
    read_expression( restriction->get_expression(), tokenizer, scope );
    tokenizer.get_token(); // consume "$."
}
//------------------------------------------------------------------------------
void read_comment(Tokenizer &tokenizer)
{
    if(tokenizer.get_token() != "$(")
        throw std::runtime_error("comment does not start with \"$(\"");

    while(tokenizer.peek() != "$)")
        tokenizer.get_token();
    /* consume "$)" */
    tokenizer.get_token();
}
//------------------------------------------------------------------------------
void read_statement(
        Metamath_database &database,
        scope &current_scope,
        Tokenizer &tokenizer)
{
    std::string label;
    if(tokenizer.peek().at(0) != '$')
        label = tokenizer.get_token();

    if(tokenizer.peek() == "$a")
    {
        read_axiom(database, scope, tokenizer, label);
    }
    else if( tokenizer.peek() == "$v" )
    {
        read_variables(database, scope, tokenizer, label);
    }
    else if( tokenizer.peek() == "${" )
    {
        if (!label.empty())
            throw std::runtime_error("Scope with label found.");
        read_scope(database, scope, tokenizer);
    }
    else if( tokenizer.peek() == "$c" )
    {
        read_constants(database, scope, tokenizer, label);
    }
    else if( tokenizer.peek() == "$f" )
    {
        read_floating_hypothesis(database, scope, tokenizer, label);
    }
    else if( tokenizer.peek() == "$e" )
    {
        read_essential_hypothesis(database, scope, tokenizer, label);
    }
    else if( tokenizer.peek() == "$p" )
    {
        read_theorem(database, scope, tokenizer, label);
    }
    else if( tokenizer.peek() == "$d" )
    {
        read_disjoint_variable_restriction(database, scope, tokenizer, label);
    }
    else if( tokenizer.peek() == "$(" )
    {
        read_comment(tokenizer);
    }
    else
    {
        throw std::runtime_error("expected label or dollar statment start");
    }
}
/*----------------------------------------------------------------------------*/
void read_database_from_file(Metamath_database &database, Tokenizer &tokenizer)
{
    Scope top_scope;
    while(!tokenizer.peek().empty())
    {
        read_statement(database, top_scope, tokenizer);
    }
}
/*----------------------------------------------------------------------------*/
} /* anonymous namespace */
/*----------------------------------------------------------------------------*/
void read_database_from_file(
        Metamath_database &database,
        std::istream &input_stream)
{
    Tokenizer tokenizer(input_stream);
    read_database_from_file(database, tokenizer);
}
/*----------------------------------------------------------------------------*/
void write_expression_to_file( const Expression &expression,
    std::ostream &output_stream )
{
    for( auto symbol : expression )
        output_stream << symbol->get_name() << ' ';
}
//------------------------------------------------------------------------------
class Statement_writer : public Const_statement_visitor
{
public:
    Statement_writer( std::ostream &output_stream );
    void operator()( const Scoping_statement * ) override;
    void operator()( const Constant_declaration * ) override;
    void operator()( const Variable_declaration * ) override;
    void operator()( const Axiom * ) override;
    void operator()( const Theorem * ) override;
    void operator()( const Essential_hypothesis * ) override;
    void operator()( const Floating_hypothesis * ) override;
    void operator()( const Disjoint_variable_restriction * ) override;
private:
    std::ostream &m_output_stream;
};
//------------------------------------------------------------------------------
Statement_writer::Statement_writer( std::ostream &output_stream ) :
    m_output_stream( output_stream )
{ }
//------------------------------------------------------------------------------
void Statement_writer::operator()( const Scoping_statement *inner_scope )
{
    m_output_stream << "${\n";
    {
        auto scoped_statement = inner_scope->get_first();
        while( scoped_statement )
        {
            Statement_writer writer( m_output_stream );
            scoped_statement->accept( writer );
            scoped_statement = scoped_statement->get_next();
        }
    }
    m_output_stream << "$}\n";
}
//------------------------------------------------------------------------------
void Statement_writer::operator()( const Constant_declaration *declaration )
{
    m_output_stream << "$c ";
    write_expression_to_file( declaration->get_expression(), m_output_stream );
    m_output_stream << "$.\n";
}
//------------------------------------------------------------------------------
void Statement_writer::operator()( const Variable_declaration *declaration )
{
    m_output_stream << "$v ";
    write_expression_to_file( declaration->get_expression(), m_output_stream );
    m_output_stream << "$.\n";
}
//------------------------------------------------------------------------------
void Statement_writer::operator()( const Axiom *axiom )
{
    m_output_stream << axiom->get_name() << " $a ";
    write_expression_to_file( axiom->get_expression(), m_output_stream );
    m_output_stream << "$.\n";
}
//------------------------------------------------------------------------------
class Proof_writer : protected Const_proof_step_visitor
{
public:
    Proof_writer( std::ostream &output_steram ) :
        m_output_stream( output_steram )
    { }
    void write( const Proof &proof )
    {
        for( auto step : proof.get_steps() )
        {
            step->accept( *this );
        }
    }

protected:
    std::ostream &m_output_stream;
};
//------------------------------------------------------------------------------
class Compressed_proof_writer : public Proof_writer
{
public:
    typedef std::map<const Statement *, int> Map_type;
    Compressed_proof_writer( std::ostream &output_stream,
        const Map_type &map
    ) :
        Proof_writer( output_stream ),
        m_statement_to_number_map( map )
    { }

private:
    void operator()( const Assertion_step *step ) override
    {
        push_statement( step->get_assertion() );
    }
    void operator()( const Essential_hypothesis_step *step ) override
    {
        push_statement( step->get_hypothesis() );
    }
    void operator()( const Floating_hypothesis_step *step ) override
    {
        push_statement( step->get_hypothesis() );
    }
    void operator()( const Add_reference_step * ) override
    {
        m_output_stream << 'Z';
    }
    void operator()( const Refer_step *step ) override
    {
        const int referred_statements_count = m_statement_to_number_map.size();
        encode_number( step->get_index() + referred_statements_count + 1 );
    }
    void operator()( const Unknown_step *step ) override
    {
        m_output_stream << '?';
    }
    void push_statement( const Statement *statement )
    {
        const auto pair = m_statement_to_number_map.find( statement );
        if( pair == m_statement_to_number_map.end() )
        {
            throw std::runtime_error( "statement outside of the reference list "
                "found when writting compressed proof" );
        }
        encode_number(pair->second + 1);
    }
    void encode_number( int number )
    {
        number--;
        if( number < 0 )
        {
            throw std::runtime_error( "n < 1" );
        }

        std::string output;
        output.insert( 0, 1, 'A'+number%20 );
        number /= 20;
        while( number > 0 )
        {
            number--;
            output.insert( 0, 1, 'U'+number%5 );
            number /= 5;
        }
        m_output_stream << output;
    }

    const Map_type &m_statement_to_number_map;
};
//------------------------------------------------------------------------------
class Compressed_proof_reference_collector : public Const_proof_step_visitor
{
public:
    typedef Compressed_proof_writer::Map_type Map_type;
    typedef std::vector<const Named_statement *> Vector_type;
    Compressed_proof_reference_collector( Map_type &map,
        Vector_type &additional_statements
    ) :
        m_statement_to_number_map( map ),
        m_additional_statements( additional_statements )
    { }
    void collect( const Proof &proof, const Theorem *theorem )
    {
        std::vector<const Named_statement *> mandatory_hypotheses;
        {
            Frame frame( theorem );
            collect_frame( frame );
            std::swap( mandatory_hypotheses, frame.get_statements() );
            mandatory_hypotheses.pop_back();
        }
        for( auto hypothesis : mandatory_hypotheses )
        {
            collect_to_map( hypothesis );
        }

        for( auto step : proof.get_steps() )
        {
            step->accept( *this );
        }
    }

private:
    void operator()( const Assertion_step *step ) override
    {
        collect_to_vector( step->get_assertion() );
        collect_to_map( step->get_assertion() );
    }
    void operator()( const Essential_hypothesis_step *step ) override
    {
        collect_to_vector( step->get_hypothesis() );
        collect_to_map( step->get_hypothesis() );
    }
    void operator()( const Floating_hypothesis_step *step ) override
    {
        collect_to_vector( step->get_hypothesis() );
        collect_to_map( step->get_hypothesis() );
    }
    void operator()( const Add_reference_step * ) override
    { }
    void operator()( const Refer_step * ) override
    { }
    void operator()( const Unknown_step * ) override
    { }
    void collect_to_map( const Named_statement *statement )
    {
        if( m_statement_to_number_map.find( statement ) ==
            m_statement_to_number_map.end() )
        {
            const int n = m_statement_to_number_map.size();
            m_statement_to_number_map.insert( std::make_pair( statement, n ) );
        }
    }
    void collect_to_vector( const Named_statement *statement )
    {
        if( m_statement_to_number_map.find( statement ) ==
            m_statement_to_number_map.end() )
        {
            m_additional_statements.push_back( statement );
        }
    }

    Map_type &m_statement_to_number_map;
    Vector_type &m_additional_statements;
};
//------------------------------------------------------------------------------
class Uncompressed_proof_writer : public Proof_writer
{
public:
    Uncompressed_proof_writer( std::ostream &output_stream ) :
        Proof_writer( output_stream )
    { }

private:
    void operator()( const Assertion_step *step ) override
    {
        write_named_statement( step->get_assertion() );
    }
    void operator()( const Essential_hypothesis_step *step ) override
    {
        write_named_statement( step->get_hypothesis() );
    }
    void operator()( const Floating_hypothesis_step *step ) override
    {
        write_named_statement( step->get_hypothesis() );
    }
    void operator()( const Add_reference_step * ) override
    {
        throw_compressed_step();
    }
    void operator()( const Refer_step * ) override
    {
        throw_compressed_step();
    }
    void operator()( const Unknown_step * )
    {
        m_output_stream << "? ";
    }
    void write_named_statement( const Named_statement *statement )
    {
        m_output_stream << statement->get_name() << ' ';
    }
    void throw_compressed_step()
    {
        throw std::runtime_error( "compressed step found in uncompressed proof,"
            " decompression on the fly not implemented yet" );
    }
};
//------------------------------------------------------------------------------
void Statement_writer::operator()( const Theorem *theorem )
{
    m_output_stream << theorem->get_name() << " $p ";
    write_expression_to_file( theorem->get_expression(), m_output_stream );
    m_output_stream << "$= ";
    if( theorem->get_proof().get_compressed() )
    {
        Compressed_proof_reference_collector::Map_type map;
        Compressed_proof_reference_collector::Vector_type additional_sentences;
        Compressed_proof_reference_collector collector( map,
            additional_sentences );
        collector.collect( theorem->get_proof(), theorem );

        // list additional statements
        m_output_stream << "( ";
        for( auto sentence : additional_sentences )
        {
            m_output_stream << sentence->get_name() << " ";
        }
        m_output_stream << ") ";

        Compressed_proof_writer writer(m_output_stream, map);
        writer.write(theorem->get_proof());
        m_output_stream << ' ';
    }
    else
    {
        Uncompressed_proof_writer writer( m_output_stream );
        writer.write( theorem->get_proof() );
    }
    m_output_stream << "$.\n";
}
//------------------------------------------------------------------------------
void Statement_writer::operator()( const Floating_hypothesis *hypothesis )
{
    m_output_stream << hypothesis->get_name() << " $f ";
    write_expression_to_file( hypothesis->get_expression(), m_output_stream );
    m_output_stream << "$.\n";
}
//------------------------------------------------------------------------------
void Statement_writer::operator()( const Essential_hypothesis *hypothesis )
{
    m_output_stream << hypothesis->get_name() << " $e ";
    write_expression_to_file( hypothesis->get_expression(), m_output_stream );
    m_output_stream << "$.\n";
}
//------------------------------------------------------------------------------
void Statement_writer::operator()( const Disjoint_variable_restriction
    *restriction )
{
    m_output_stream << "$d ";
    write_expression_to_file( restriction->get_expression(), m_output_stream );
    m_output_stream << "$.\n";
}
//------------------------------------------------------------------------------
void write_database_to_file( const Metamath_database &db, std::ostream
    &output_stream )
{
    auto scoped_statement = db.get_top_scope()->get_first();
    while( scoped_statement )
    {
        Statement_writer writer( output_stream );
        scoped_statement->accept( writer );
        scoped_statement = scoped_statement->get_next();
    }
}
/*----------------------------------------------------------------------------*/
} /* namespace metamath_playground */
