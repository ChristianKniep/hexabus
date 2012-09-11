#include "hba_output.hpp"

#include <libhbc/error.hpp>

using namespace hexabus;

void HBAOutput::operator()(std::ostream& ostr) {
  // TODO
  // - find highest state machine index
  //   - this will work differently later, when the machines are partitioned, but for now we assume that each state machine only controls one device
  // - go through all the state machines and translate them
  //   - states to states
  //   - if-conditions to conditions
  //     - this also needs more thinking!! (*)
  //   - write actions to actions
  //     - more thinking: What happens on multiple writes? (*)
  //
  // *) These should be done on graph level, along with the slicing/partitoning.


  // For now, just assume we only work on machine index 0 (single device, single state machine)
  // TODO once this works, extend.

  ostr << "# Auto-generated by Hexbaus Compiler" << std::endl; // TODO nicer looking message
  ostr << std::endl;

  ostr << "startstate init;" << std::endl << std::endl;

  // iterate over vertices, find condition vertices and create condition blocks for them.
  graph_t::vertex_iterator vertexIt, vertexEnd;
  boost::tie(vertexIt, vertexEnd) = vertices((*_g));
  for (; vertexIt != vertexEnd; ++vertexIt) {
    vertex_id_t vertexID = *vertexIt;
    vertex_t& vertex = (*_g)[vertexID];

    if (vertex.type == v_cond /* TODO */ && vertex.machine_id == 0) {
      try {
        condition_doc cond = boost::get<condition_doc>(vertex.contents);
        try {
          atomic_condition_doc at_cond = boost::get<atomic_condition_doc>(cond);

          // give the condition a name
          ostr << "condition cond_" << vertex.machine_id << "_" << vertex.vertex_id << " {" << std::endl;

          // find the device alias, print its IP address
          try {
            device_table::iterator d_it = _d->find(boost::get<std::string>(at_cond.geid.device_alias));
            if(d_it == _d->end()) {
              // TODO this is an error in the input file
              std::ostringstream oss;
              oss << "Device name not found: " << boost::get<std::string>(at_cond.geid.device_alias);
              throw HBAConversionErrorException(oss.str());
            }

            ostr << "  ip := " << d_it->second.ipv6_address << ";" << std::endl;

          } catch(boost::bad_get e) {
            // TODO this is an error in the input file
            std::ostringstream oss;
            oss << "Only literal device names (no placeholders) allowed in state machine definition!" << std::endl;
            throw HBAConversionErrorException(oss.str());
          }

          // find the endpoint name, print its IP address
          try {
            endpoint_table::iterator e_it = _e->find(boost::get<std::string>(at_cond.geid.endpoint_name));
            if(e_it == _e->end()) {
              // TODO this is an error in the input file
              std::ostringstream oss;
              oss << "Endpoint name not found: " << boost::get<std::string>(at_cond.geid.endpoint_name) << std::endl;
              throw HBAConversionErrorException(oss.str());
            }

            ostr << "  eid := " << e_it->second.eid << ";" << std::endl;

          } catch(boost::bad_get e) {
            // TODO this is an error in the input file
            std::ostringstream oss;
            oss << "Only literal endpoint names (no placeholders) allowed in state machine definition!" << std::endl;
            throw HBAConversionErrorException(oss.str());
          }

          // value comparison...
          ostr << "  value ";

          // comparison operator
          switch(at_cond.comp_op) {
            case STM_EQ:
              ostr << "==";
              break;
            case STM_LEQ:
              ostr << "<=";
              break;
            case STM_GEQ:
              ostr << ">=";
              break;
            case STM_LT:
              ostr << "<";
              break;
            case STM_GT:
              ostr << ">";
              break;
            case STM_NEQ:
              ostr << "!=";
              break;
            default:
              throw HBAConversionErrorException("operator not implemeted");
              break;
          }

          // constant
          try {
            ostr << " " << boost::get<float>(at_cond.constant) << ";" << std::endl;;
          } catch(boost::bad_get e) {
            // TODO this is an error in the input file
            std::ostringstream oss;
            oss << "Only literal constants (no placeholders) allowed in state machine definition!" << std::endl;
            oss << "(At the moment, only float is allowed!)" << std::endl; // TODO !!!
            throw HBAConversionErrorException(oss.str());
          }

          // closing bracket
          ostr << "}" << std::endl << std::endl;

        } catch(boost::bad_get e) {
          // TODO this is a not-implemented-yet error
          throw HBAConversionErrorException("Only atomic conditions implemented at the moment :(");
        }
      } catch(boost::bad_get e) {
        // TODO this is a hexabus-compiler-has-a-bug error
        std::ostringstream oss;
        oss << "Condition vertex does not contain condition data!" << std::endl;
        throw HBAConversionErrorException(oss.str());
      }
    }
  }

  // iterate over vertices again, find state vertices and create state blocks for them.
  boost::tie(vertexIt, vertexEnd) = vertices((*_g));
  for (; vertexIt != vertexEnd; ++vertexIt) {
    vertex_id_t vertexID = *vertexIt;
    vertex_t & vertex = (*_g)[vertexID];

    if(vertex.type == v_state /* TODO */ && vertex.machine_id == 0) {
      // state name
      ostr << "state_" << vertex.machine_id << "_" << vertex.vertex_id << " {" << std::endl;

      // if-blocks in the state
      graph_t::out_edge_iterator outEdgeIt, outEdgeEnd;
      boost::tie(outEdgeIt, outEdgeEnd) = out_edges(vertexID, (*_g));
      for(; outEdgeIt != outEdgeEnd; outEdgeIt++) {
        edge_id_t edgeID = *outEdgeIt;
        // edge_t& edge = (*_g)[edgeID];

        // "edge" now connects our state to an if-block.
        // get the condition and the command block from there
        vertex_id_t if_vertexID = edgeID.m_target;
        vertex_t& if_vertex = (*_g)[if_vertexID];

        if(if_vertex.type != v_cond) // TODO this could be a bug in hexabus compiler
          throw HBAConversionErrorException("Condition vertex expected. Other vertex type found.");

        ostr << "  if " << "cond_" << if_vertex.machine_id << "_" << if_vertex.vertex_id << " {" << std::endl;

        // now find the command block connected to the if block
        // TODO make sure each if-block has exactly one command block, and the command block has exactly one command
        // (do this when/after slicing the program
        graph_t::out_edge_iterator commandEdgeIt, commandEdgeEnd;
        boost::tie(commandEdgeIt, commandEdgeEnd) = out_edges(if_vertexID, (*_g));

        // TODO make sure there is xactly one, exception if not
        edge_id_t commandID = *commandEdgeIt;
        // edge_t& command_edge = (*_g)[commandID];
        vertex_id_t command_vertexID = commandID.m_target;
        vertex_t& command_vertex = (*_g)[command_vertexID];

        if(command_vertex.type != v_command) // TODO this could be a bug in hexabus compiler
          throw HBAConversionErrorException("Command vertex expected. Other vertex type found.");

        // find the write command
        try {
          command_block_doc command = boost::get<command_block_doc>(command_vertex.contents);
          write_command_doc& write_cmd = command.commands[0].write_command; // TODO make sure there is exactly one.

          // TODO
          // Here some checks could be done:
          // Is the endpoint we are writing to write-able?
          // Does the endpoint on whose data we rely in the if-clause broadcast?
          // Do the datatypes fit?
          // Are we creating non-leavable states (should be checked earlier though)?
          // ...

          // now find the eid
          unsigned int eid;
          try {
            std::string epname = boost::get<std::string>(write_cmd.geid.endpoint_name);
            endpoint_table::iterator ep_it = _e->find(epname);
            if(ep_it == _e->end()) {
              // TODO this is an error in the input file
              std::ostringstream oss;
              oss << "Endpoint name not found: " << boost::get<std::string>(write_cmd.geid.endpoint_name) << std::endl;
              throw HBAConversionErrorException(oss.str());
            }

            eid = ep_it->second.eid;
          } catch (boost::bad_get e) {
            // TODO this is a programming error
            throw HBAConversionErrorException("Only literal endpoint names allowed in state machine definitions.");
          }

          // now find the value
          float value;
          try {
            value = boost::get<float>(write_cmd.constant);
          } catch(boost::bad_get e) {
            // TODO
            throw HBAConversionErrorException("Only literal constants allowed in state machine definition. (At the moment only float works)");
          }

          // output the "set" line
          ostr << "    set " << eid << " := " << value << ";" << std::endl;
        } catch(boost::bad_get e) {
          // TODO this is a hexabus-compiler-has-a-bug error
          throw HBAConversionErrorException("Command vertex does not contain commamnd data!");
        }

        // now follow the edge to the target state vertex
        graph_t::out_edge_iterator targetEdgeIt, targetEdgeEnd;
        boost::tie(targetEdgeIt, targetEdgeEnd) = out_edges(command_vertexID, (*_g));

        // TODO make sure we have exactly one...
        edge_id_t targetID = *targetEdgeIt;
        vertex_id_t target_state_vertexID = targetID.m_target;
        vertex_t& target_state_vertex = (*_g)[target_state_vertexID];

        if(target_state_vertex.type != v_state) // TODO this is probably a bug in hexbaus compiler
          throw HBAConversionErrorException("State vertex expected. Got other vertex type.");

        // print goodstate / badstate line
        ostr << "    goodstate " << "state_" << target_state_vertex.machine_id << "_" << target_state_vertex.vertex_id << ";" << std::endl;
        // TODO for now, badstate = goodstate
        ostr << "    badstate " << "state_" << target_state_vertex.machine_id << "_" << target_state_vertex.vertex_id << ";" << std::endl;

        // closing bracket
        ostr << "  }" << std::endl;
      }

      // closing bracket
      ostr << "}" << std::endl << std::endl;
    }
  }
}

