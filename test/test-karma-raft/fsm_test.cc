#include "karma-raft/fsm.h"
#define BOOST_TEST_MODULE KARMA_RAFT_TEST
#include <boost/test/unit_test.hpp>
#include <sys/types.h>
#include <vector>
#include "karma-raft/common.h"
#include "karma-raft/log.h"
#include "karma-raft/tracker.h"
#include "test/test-karma-raft/helper.h"
#define BOOST_TEST_MODULE KARMA_RAFT_TEST
namespace test {
    BOOST_AUTO_TEST_SUITE (FsmTest)
    
    BOOST_AUTO_TEST_CASE(test_votes) {
        auto id1 = id();

        raft::votes votes(config_from_ids({id1}));
        BOOST_CHECK_EQUAL(votes.tally_votes(), raft::vote_result::UNKNOWN);
        BOOST_CHECK_EQUAL(votes.voters().size(), 1);
        // Try a vote from an unknown server, it should be ignored.
        votes.register_vote(id(), true);
        votes.register_vote(id1, false);
        // Quorum votes against the decision
        BOOST_CHECK_EQUAL(votes.tally_votes(), raft::vote_result::LOST);
        // Another vote from the same server is ignored
        votes.register_vote(id1, true);
        votes.register_vote(id1, true);
        BOOST_CHECK_EQUAL(votes.tally_votes(), raft::vote_result::LOST);
        auto id2 = id();
        votes = raft::votes(config_from_ids({id1, id2}));
        BOOST_CHECK_EQUAL(votes.voters().size(), 2);
        votes.register_vote(id1, true);
        // We need a quorum of participants to win an election
        BOOST_CHECK_EQUAL(votes.tally_votes(), raft::vote_result::UNKNOWN);
        votes.register_vote(id2, false);
        // At this point it's clear we don't have enough votes
        BOOST_CHECK_EQUAL(votes.tally_votes(), raft::vote_result::LOST);
        auto id3 = id();
        // Joint configuration
        votes = raft::votes(raft::configuration(config_set({id1}), config_set({id2, id3})));
        BOOST_CHECK_EQUAL(votes.voters().size(), 3);
        BOOST_CHECK_EQUAL(votes.tally_votes(), raft::vote_result::UNKNOWN);
        votes.register_vote(id2, true);
        votes.register_vote(id3, true);
        BOOST_CHECK_EQUAL(votes.tally_votes(), raft::vote_result::UNKNOWN);
        votes.register_vote(id1, false);
        BOOST_CHECK_EQUAL(votes.tally_votes(), raft::vote_result::LOST);
        votes = raft::votes(raft::configuration(config_set({id1}), config_set({id2, id3})));
        votes.register_vote(id2, true);
        votes.register_vote(id3, true);
        votes.register_vote(id1, true);
        BOOST_CHECK_EQUAL(votes.tally_votes(), raft::vote_result::WON);
        votes = raft::votes(raft::configuration(config_set({id1, id2, id3}), config_set({id1})));
        BOOST_CHECK_EQUAL(votes.voters().size(), 3);
        votes.register_vote(id1, true);
        BOOST_CHECK_EQUAL(votes.tally_votes(), raft::vote_result::UNKNOWN);
        // This gives us a majority in both new and old
        // configurations.
        votes.register_vote(id2, true);
        BOOST_CHECK_EQUAL(votes.tally_votes(), raft::vote_result::WON);
        // Basic voting test for 4 nodes
        auto id4 = id();
        votes = raft::votes(config_from_ids({id1, id2, id3, id4}));
        votes.register_vote(id1, true);
        votes.register_vote(id2, true);
        BOOST_CHECK_EQUAL(votes.tally_votes(), raft::vote_result::UNKNOWN);
        votes.register_vote(id3, false);
        BOOST_CHECK_EQUAL(votes.tally_votes(), raft::vote_result::UNKNOWN);
        votes.register_vote(id4, false);
        BOOST_CHECK_EQUAL(votes.tally_votes(), raft::vote_result::LOST);
        auto id5 = id();
        // Basic voting test for 5 nodes
        votes = raft::votes(raft::configuration(config_set({id1, id2, id3, id4, id5}), config_set({id1, id2, id3})));
        votes.register_vote(id1, false);
        votes.register_vote(id2, false);
        BOOST_CHECK_EQUAL(votes.tally_votes(), raft::vote_result::LOST);
        votes.register_vote(id3, true);
        votes.register_vote(id4, true);
        votes.register_vote(id5, true);
        BOOST_CHECK_EQUAL(votes.tally_votes(), raft::vote_result::LOST);
        // Basic voting test with tree voters and one no-voter
        votes = raft::votes(raft::configuration({
                {server_addr_from_id(id1), true}, {server_addr_from_id(id2), true},
                {server_addr_from_id(id3), true}, {server_addr_from_id(id4), false}}));
        votes.register_vote(id1, true);
        votes.register_vote(id2, true);
        BOOST_CHECK_EQUAL(votes.tally_votes(), raft::vote_result::WON);
        // Basic test that non-voting votes are ignored
        votes = raft::votes(raft::configuration({
                {server_addr_from_id(id1), true}, {server_addr_from_id(id2), true},
                {server_addr_from_id(id3), true}, {server_addr_from_id(id4), false}}));
        votes.register_vote(id1, true);
        votes.register_vote(id4, true);
        BOOST_CHECK_EQUAL(votes.tally_votes(), raft::vote_result::UNKNOWN);
        votes.register_vote(id3, true);
        BOOST_CHECK_EQUAL(votes.tally_votes(), raft::vote_result::WON);
        // Joint configuration with non voting members
        votes = raft::votes(raft::configuration(
                {{server_addr_from_id(id1), true}},
                {{server_addr_from_id(id2), true}, {server_addr_from_id(id3), true}, {server_addr_from_id(id4), false}}));
        BOOST_CHECK_EQUAL(votes.voters().size(), 3);
        BOOST_CHECK_EQUAL(votes.tally_votes(), raft::vote_result::UNKNOWN);
        votes.register_vote(id2, true);
        votes.register_vote(id3, true);
        votes.register_vote(id4, true);
        BOOST_CHECK_EQUAL(votes.tally_votes(), raft::vote_result::UNKNOWN);
        votes.register_vote(id1, true);
        BOOST_CHECK_EQUAL(votes.tally_votes(), raft::vote_result::WON);
        // Same node is voting in one config and non voting in another
        votes = raft::votes(raft::configuration(
                {{server_addr_from_id(id1), true}, {server_addr_from_id(id4), true}},
                {{server_addr_from_id(id2), true}, {server_addr_from_id(id3), true}, {server_addr_from_id(id4), false}}));
        votes.register_vote(id2, true);
        votes.register_vote(id1, true);
        votes.register_vote(id4, true);
        BOOST_CHECK_EQUAL(votes.tally_votes(), raft::vote_result::UNKNOWN);
        votes.register_vote(id3, true);
        BOOST_CHECK_EQUAL(votes.tally_votes(), raft::vote_result::WON);
        // std::cout << "fuck you" << std::endl;
    }   
    BOOST_AUTO_TEST_CASE(test_tracker) {
        auto id1 = id();
        raft::tracker tracker;
        raft::configuration cfg = config_from_ids({id1});
        tracker.set_configuration(cfg, 1);
        BOOST_CHECK_NE(tracker.find(id1), nullptr);
        // The node with id set during construction is assumed to be
        // the leader, since otherwise we wouldn't create a tracker
        // in the first place.
        BOOST_CHECK_EQUAL(tracker.committed(raft::index_t(0)), 0);
        // Avoid keeping a reference, follower_progress address may
        // change with configuration change
        auto pr = [&tracker](raft::server_id id) -> raft::follower_progress* {
            return tracker.find(id);
        };
        BOOST_CHECK_EQUAL(pr(id1)->m_match_idx, 0);
        BOOST_CHECK_EQUAL(pr(id1)->m_next_idx, 1);

        pr(id1)->accepted(1);
        BOOST_CHECK_EQUAL(pr(id1)->m_match_idx, 1);
        BOOST_CHECK_EQUAL(pr(id1)->m_next_idx, 2);
        BOOST_CHECK_EQUAL(tracker.committed(raft::index_t(0)), 1);

        pr(id1)->accepted(10);
        BOOST_CHECK_EQUAL(pr(id1)->m_match_idx, 10);
        BOOST_CHECK_EQUAL(pr(id1)->m_next_idx, 11);
        BOOST_CHECK_EQUAL(tracker.committed(raft::index_t(0)), 10);

        // Out of order confirmation is OK
        //
        pr(id1)->accepted(5);
        BOOST_CHECK_EQUAL(pr(id1)->m_match_idx, 10);
        BOOST_CHECK_EQUAL(pr(id1)->m_next_idx, 11);
        BOOST_CHECK_EQUAL(tracker.committed(raft::index_t(5)), 10);

        // Enter joint configuration {A,B,C}
        auto id2 = id(), id3 = id();
        cfg.enter_joint(config_set({id1, id2, id3}));
        tracker.set_configuration(cfg, 1);
        BOOST_CHECK_EQUAL(tracker.committed(raft::index_t(10)), 10);
        pr(id2)->accepted(11);
        BOOST_CHECK_EQUAL(tracker.committed(raft::index_t(10)), 10);
        pr(id3)->accepted(12);
        BOOST_CHECK_EQUAL(tracker.committed(raft::index_t(10)), 10);
        pr(id1)->accepted(13);
        BOOST_CHECK_EQUAL(tracker.committed(raft::index_t(10)), 12);
        pr(id1)->accepted(14);
        BOOST_CHECK_EQUAL(tracker.committed(raft::index_t(13)), 13);

        // Leave joint configuration, final configuration is  {A,B,C}
        cfg.leave_joint();
        tracker.set_configuration(cfg, 1);
        BOOST_CHECK_EQUAL(tracker.committed(raft::index_t(13)), 13);

        auto id4 = id(), id5 = id();
        cfg.enter_joint(config_set({id3, id4, id5}));
        tracker.set_configuration(cfg, 1);
        BOOST_CHECK_EQUAL(tracker.committed(raft::index_t(13)), 13);
        pr(id1)->accepted(15);
        BOOST_CHECK_EQUAL(tracker.committed(raft::index_t(13)), 13);
        pr(id5)->accepted(15);
        BOOST_CHECK_EQUAL(tracker.committed(raft::index_t(13)), 13);
        pr(id3)->accepted(15);
        BOOST_CHECK_EQUAL(tracker.committed(raft::index_t(13)), 15);
        // This does not advance the joint quorum
        pr(id1)->accepted(16);
        pr(id4)->accepted(17);
        pr(id5)->accepted(18);
        BOOST_CHECK_EQUAL(tracker.committed(raft::index_t(15)), 15);

        cfg.leave_joint();
        tracker.set_configuration(cfg, 1);
        // Leaving joint configuration commits more entries
        BOOST_CHECK_EQUAL(tracker.committed(raft::index_t(15)), 17);
        //
        cfg.enter_joint(config_set({id1}));
        cfg.leave_joint();
        cfg.enter_joint(config_set({id2}));
        tracker.set_configuration(cfg, 1);
        // Sic: we're in a weird state. The joint commit index
        // is actually 1, since id2 is at position 1. But in
        // unwinding back the commit index would be weird,
        // so we report back the hint (prev_commit_idx).
        // As soon as the cluster enters joint configuration,
        // and old quorum is insufficient, the leader won't be able to
        // commit new entries until the new members catch up.
        BOOST_CHECK_EQUAL(tracker.committed(raft::index_t(17)), 17);
        pr(id1)->accepted(18);
        BOOST_CHECK_EQUAL(tracker.committed(raft::index_t(17)), 17);
        pr(id2)->accepted(19);
        BOOST_CHECK_EQUAL(tracker.committed(raft::index_t(17)), 18);
        pr(id1)->accepted(20);
        BOOST_CHECK_EQUAL(tracker.committed(raft::index_t(18)), 19);

        // Check that non voting member is not counted for the quorum in simple config
        cfg.enter_joint({{server_addr_from_id(id1), true}, {server_addr_from_id(id2), true}, {server_addr_from_id(id3), false}});
        cfg.leave_joint();
        tracker.set_configuration(cfg, 1);
        pr(id1)->accepted(30);
        pr(id2)->accepted(25);
        pr(id3)->accepted(30);
        BOOST_CHECK_EQUAL(tracker.committed(raft::index_t(0)), 25);

        // Check that non voting member is not counted for the quorum in joint config
        cfg.enter_joint({{server_addr_from_id(id4), true}, {server_addr_from_id(id5), true}});
        tracker.set_configuration(cfg, 1);
        pr(id4)->accepted(30);
        pr(id5)->accepted(30);
        BOOST_CHECK_EQUAL(tracker.committed(raft::index_t(0)), 25);

        // Check the case where the same node is in both config but different voting rights
        cfg.leave_joint();
        cfg.enter_joint({{server_addr_from_id(id1), true}, {server_addr_from_id(id2), true}, {server_addr_from_id(id5), false}});
        BOOST_CHECK_EQUAL(tracker.committed(raft::index_t(0)), 25);
    }

    BOOST_AUTO_TEST_CASE(test_log_last_conf_idx) {
        // last_conf_idx, prev_conf_idx are initialized correctly,
        // and maintained during truncate head/truncate tail
        raft::server_id id1 = id();
        raft::configuration cfg = config_from_ids({id1});
        raft::log log{raft::snapshot_descriptor{.config = cfg}};
        BOOST_CHECK_EQUAL(log.last_conf_idx(), 0);
        add_entry(log, cfg);
        BOOST_CHECK_EQUAL(log.last_conf_idx(), 1);
        add_entry(log, raft::log_entry::dummy{});
        add_entry(log, cfg);
        BOOST_CHECK_EQUAL(log.last_conf_idx(), 3);
        // apply snapshot truncates the log and resets last_conf_idx()
        log.apply_snapshot(log_snapshot(log, log.last_idx()), 0, 0);
        BOOST_CHECK_EQUAL(log.last_conf_idx(), log.get_snapshot().idx);
        // log::last_term() is maintained correctly by truncate_head/truncate_tail() (snapshotting)
        BOOST_CHECK_EQUAL(log.last_term(), log.get_snapshot().term);
        BOOST_CHECK(log.term_for(log.get_snapshot().idx));
        BOOST_CHECK_EQUAL(log.term_for(log.get_snapshot().idx).value(), log.get_snapshot().term);
        BOOST_CHECK(! log.term_for(log.last_idx() - raft::index_t(1)));
        add_entry(log, raft::log_entry::dummy{});
        BOOST_CHECK(log.term_for(log.last_idx()));
        add_entry(log, raft::log_entry::dummy{});
        const size_t GAP = 10;
        // apply_snapshot with a log gap, this should clear all log
        // entries, despite that trailing is given, a gap
        // between old log entries and a snapshot would violate
        // log continuity.
        log.apply_snapshot(log_snapshot(log, log.last_idx() + raft::index_t(GAP)), GAP * 2, std::numeric_limits<size_t>::max());
        BOOST_CHECK(log.empty());
        BOOST_CHECK_EQUAL(log.next_idx(), log.get_snapshot().idx + raft::index_t(1));
        add_entry(log, raft::log_entry::dummy{});
        BOOST_CHECK_EQUAL(log.in_memory_size(), 1);
        add_entry(log, raft::log_entry::dummy{});
        BOOST_CHECK_EQUAL(log.in_memory_size(), 2);
        // Set trailing longer than the length of the log.
        log.apply_snapshot(log_snapshot(log, log.last_idx()), 3, std::numeric_limits<size_t>::max());
        BOOST_CHECK_EQUAL(log.in_memory_size(), 2);
        // Set trailing the same length as the current log length
        add_entry(log, raft::log_entry::dummy{});
        BOOST_CHECK_EQUAL(log.in_memory_size(), 3);
        log.apply_snapshot(log_snapshot(log, log.last_idx()), 3, std::numeric_limits<size_t>::max());
        BOOST_CHECK_EQUAL(log.in_memory_size(), 3);
        BOOST_CHECK_EQUAL(log.last_conf_idx(), log.get_snapshot().idx);
        add_entry(log, raft::log_entry::dummy{});
        // Set trailing shorter than the length of the log
        log.apply_snapshot(log_snapshot(log, log.last_idx()), 1, std::numeric_limits<size_t>::max());
        BOOST_CHECK_EQUAL(log.in_memory_size(), 1);
        // check that configuration from snapshot is used and not config entries from a trailing
        add_entry(log, cfg);
        add_entry(log, cfg);
        add_entry(log, raft::log_entry::dummy{});
        auto snp_idx = log.last_idx();
        log.apply_snapshot(log_snapshot(log, snp_idx), 10, std::numeric_limits<size_t>::max());
        BOOST_CHECK_EQUAL(log.last_conf_idx(), snp_idx);
        // Check that configuration from the log is used if it has higher index then snapshot idx
        add_entry(log, raft::log_entry::dummy{});
        snp_idx = log.last_idx();
        add_entry(log, cfg);
        add_entry(log, cfg);
        log.apply_snapshot(log_snapshot(log, snp_idx), 10, std::numeric_limits<size_t>::max());
        BOOST_CHECK_EQUAL(log.last_conf_idx(), log.last_idx());
    }

    void test_election_single_node_helper(raft::fsm_config fcfg) {

        raft::server_id id1 = id();
        raft::configuration cfg = config_from_ids({id1});
        raft::log log{raft::snapshot_descriptor{.config = cfg}};
        raft::fsm fsm(id1, raft::term_t(), raft::server_id(), std::move(log), trivial_failure_detector, fcfg);

        election_timeout(fsm);

        // Immediately converts from leader to follower if quorum=1
        BOOST_CHECK(fsm.is_leader());

        auto output = fsm.get_output();

        BOOST_CHECK(output.term_and_vote);
        BOOST_CHECK(output.term_and_vote->first);
        BOOST_CHECK(output.term_and_vote->second);
        BOOST_CHECK(output.messages.empty());
        // A new leader applies one dummy entry
        BOOST_CHECK_EQUAL(output.log_entries.size(), 1);
        if (output.log_entries.size()) {
            BOOST_CHECK(std::holds_alternative<raft::log_entry::dummy>(output.log_entries[0]->data));
        }
        BOOST_CHECK(output.committed_entries.empty());
        // The leader does not become candidate simply because
        // a timeout has elapsed, i.e. there are no spurious
        // elections.
        election_timeout(fsm);
        BOOST_CHECK(fsm.is_leader());
        output = fsm.get_output();
        BOOST_CHECK(!output.term_and_vote);
        BOOST_CHECK(output.messages.empty());
        BOOST_CHECK(output.log_entries.empty());
        // Dummy entry is now committed
        BOOST_CHECK_EQUAL(output.committed_entries.size(), 1);
        if (output.committed_entries.size()) {
            BOOST_CHECK(std::holds_alternative<raft::log_entry::dummy>(output.committed_entries[0]->data));
        }
    }
    BOOST_AUTO_TEST_CASE(test_election_single_node) {
        test_election_single_node_helper(fsm_cfg);
    }
    BOOST_AUTO_TEST_CASE(test_single_node_is_quiet) {

        raft::server_id id1 = id();
        raft::configuration cfg = config_from_ids({id1});
        raft::log log{raft::snapshot_descriptor{.config = cfg}};

        auto fsm = create_follower(id1, std::move(log));

        election_timeout(fsm);

        // Immediately converts from leader to follower if quorum=1
        BOOST_CHECK(fsm.is_leader());

        (void) fsm.get_output();

        fsm.add_entry(raft::command_t{});

        BOOST_CHECK(fsm.get_output().messages.empty());

        fsm.tick();

        BOOST_CHECK(fsm.get_output().messages.empty());
    }
    BOOST_AUTO_TEST_CASE(test_snapshot_follower_is_quiet) {
        // raft::server_id id1 = id(), id2 = id();

        // raft::configuration cfg = config_from_ids({id1, id2});
        // raft::log log(raft::snapshot_descriptor{.idx = raft::index_t{999}, .config = cfg});

        // // log.emplace_back(seastar::make_lw_shared<raft::log_entry>(raft::log_entry{term_t{10}, index_t{1000}}));
        // // raft::log log{raft::snapshot_descriptor{.config = cfg}};

        // log.stable_to(log.last_idx());

        // fsm_debug fsm(id1, term_t{10}, server_id{}, std::move(log), trivial_failure_detector, fsm_cfg);

        // // become leader
        // election_timeout(fsm);

        // fsm.step(id2, raft::vote_reply{fsm.get_current_term(), true});

        // BOOST_CHECK(fsm.is_leader());

        // // clear output
        // (void) fsm.get_output();

        // // reply with reject pointing into the snapshot
        // fsm.step(id2, raft::append_reply{fsm.get_current_term(), raft::index_t{1}, raft::append_reply::rejected{raft::index_t{1000}, raft::index_t{1}}});

        // BOOST_CHECK(fsm.get_progress(id2).state == raft::follower_progress::state::SNAPSHOT);

        // // clear output
        // (void) fsm.get_output();

        // for (int i = 0; i < 100; i++) {
        // fsm.tick();
        // BOOST_CHECK(fsm.get_output().messages.empty());
        // }
    }
    BOOST_AUTO_TEST_SUITE_END()
}