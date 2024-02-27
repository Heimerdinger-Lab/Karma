// automatically generated by the FlatBuffers compiler, do not modify


#ifndef FLATBUFFERS_GENERATED_RPC_KARMA_RPC_H_
#define FLATBUFFERS_GENERATED_RPC_KARMA_RPC_H_

#include "flatbuffers/flatbuffers.h"

// Ensure the included flatbuffers.h is the same version as when this file was
// generated, otherwise it may not be compatible.
static_assert(FLATBUFFERS_VERSION_MAJOR == 23 &&
              FLATBUFFERS_VERSION_MINOR == 5 &&
              FLATBUFFERS_VERSION_REVISION == 26,
             "Non-compatible flatbuffers version included");

namespace karma_rpc {

struct AppendEntryRequest;
struct AppendEntryRequestBuilder;

struct LogEntry;
struct LogEntryBuilder;

struct AppendEntryRejected;
struct AppendEntryRejectedBuilder;

struct AppendEntryAccepted;
struct AppendEntryAcceptedBuilder;

struct AppendEntryReply;
struct AppendEntryReplyBuilder;

struct VoteRequest;
struct VoteRequestBuilder;

struct VoteReply;
struct VoteReplyBuilder;

struct TimeOut;
struct TimeOutBuilder;

struct ReadQuorum;
struct ReadQuorumBuilder;

struct ReadQuorumReply;
struct ReadQuorumReplyBuilder;

struct EchoRequest;
struct EchoRequestBuilder;

struct EchoReply;
struct EchoReplyBuilder;

enum OperationCode : int16_t {
  OperationCode_UNKNOW = 0,
  OperationCode_ECHO = 1,
  OperationCode_HEARTBEAT = 2,
  OperationCode_APPEND_ENTRY = 3,
  OperationCode_VOTE = 4,
  OperationCode_TIME_OUT = 5,
  OperationCode_READ_QUORUM = 6,
  OperationCode_MIN = OperationCode_UNKNOW,
  OperationCode_MAX = OperationCode_READ_QUORUM
};

inline const OperationCode (&EnumValuesOperationCode())[7] {
  static const OperationCode values[] = {
    OperationCode_UNKNOW,
    OperationCode_ECHO,
    OperationCode_HEARTBEAT,
    OperationCode_APPEND_ENTRY,
    OperationCode_VOTE,
    OperationCode_TIME_OUT,
    OperationCode_READ_QUORUM
  };
  return values;
}

inline const char * const *EnumNamesOperationCode() {
  static const char * const names[8] = {
    "UNKNOW",
    "ECHO",
    "HEARTBEAT",
    "APPEND_ENTRY",
    "VOTE",
    "TIME_OUT",
    "READ_QUORUM",
    nullptr
  };
  return names;
}

inline const char *EnumNameOperationCode(OperationCode e) {
  if (::flatbuffers::IsOutRange(e, OperationCode_UNKNOW, OperationCode_READ_QUORUM)) return "";
  const size_t index = static_cast<size_t>(e);
  return EnumNamesOperationCode()[index];
}

enum AppendEntryResult : uint8_t {
  AppendEntryResult_NONE = 0,
  AppendEntryResult_AppendEntryAccepted = 1,
  AppendEntryResult_AppendEntryRejected = 2,
  AppendEntryResult_MIN = AppendEntryResult_NONE,
  AppendEntryResult_MAX = AppendEntryResult_AppendEntryRejected
};

inline const AppendEntryResult (&EnumValuesAppendEntryResult())[3] {
  static const AppendEntryResult values[] = {
    AppendEntryResult_NONE,
    AppendEntryResult_AppendEntryAccepted,
    AppendEntryResult_AppendEntryRejected
  };
  return values;
}

inline const char * const *EnumNamesAppendEntryResult() {
  static const char * const names[4] = {
    "NONE",
    "AppendEntryAccepted",
    "AppendEntryRejected",
    nullptr
  };
  return names;
}

inline const char *EnumNameAppendEntryResult(AppendEntryResult e) {
  if (::flatbuffers::IsOutRange(e, AppendEntryResult_NONE, AppendEntryResult_AppendEntryRejected)) return "";
  const size_t index = static_cast<size_t>(e);
  return EnumNamesAppendEntryResult()[index];
}

template<typename T> struct AppendEntryResultTraits {
  static const AppendEntryResult enum_value = AppendEntryResult_NONE;
};

template<> struct AppendEntryResultTraits<karma_rpc::AppendEntryAccepted> {
  static const AppendEntryResult enum_value = AppendEntryResult_AppendEntryAccepted;
};

template<> struct AppendEntryResultTraits<karma_rpc::AppendEntryRejected> {
  static const AppendEntryResult enum_value = AppendEntryResult_AppendEntryRejected;
};

bool VerifyAppendEntryResult(::flatbuffers::Verifier &verifier, const void *obj, AppendEntryResult type);
bool VerifyAppendEntryResultVector(::flatbuffers::Verifier &verifier, const ::flatbuffers::Vector<::flatbuffers::Offset<void>> *values, const ::flatbuffers::Vector<uint8_t> *types);

struct AppendEntryRequest FLATBUFFERS_FINAL_CLASS : private ::flatbuffers::Table {
  typedef AppendEntryRequestBuilder Builder;
  enum FlatBuffersVTableOffset FLATBUFFERS_VTABLE_UNDERLYING_TYPE {
    VT_FROM_ID = 4,
    VT_GROUP_ID = 6,
    VT_CURRENT_TERM = 8,
    VT_PREV_LOG_IDX = 10,
    VT_PREV_LOG_TERM = 12,
    VT_LEADER_COMMIT_IDX = 14,
    VT_ENTRIES = 16
  };
  int64_t from_id() const {
    return GetField<int64_t>(VT_FROM_ID, -1LL);
  }
  int64_t group_id() const {
    return GetField<int64_t>(VT_GROUP_ID, -1LL);
  }
  int64_t current_term() const {
    return GetField<int64_t>(VT_CURRENT_TERM, -1LL);
  }
  int64_t prev_log_idx() const {
    return GetField<int64_t>(VT_PREV_LOG_IDX, -1LL);
  }
  int64_t prev_log_term() const {
    return GetField<int64_t>(VT_PREV_LOG_TERM, -1LL);
  }
  int64_t leader_commit_idx() const {
    return GetField<int64_t>(VT_LEADER_COMMIT_IDX, -1LL);
  }
  const ::flatbuffers::Vector<::flatbuffers::Offset<karma_rpc::LogEntry>> *entries() const {
    return GetPointer<const ::flatbuffers::Vector<::flatbuffers::Offset<karma_rpc::LogEntry>> *>(VT_ENTRIES);
  }
  bool Verify(::flatbuffers::Verifier &verifier) const {
    return VerifyTableStart(verifier) &&
           VerifyField<int64_t>(verifier, VT_FROM_ID, 8) &&
           VerifyField<int64_t>(verifier, VT_GROUP_ID, 8) &&
           VerifyField<int64_t>(verifier, VT_CURRENT_TERM, 8) &&
           VerifyField<int64_t>(verifier, VT_PREV_LOG_IDX, 8) &&
           VerifyField<int64_t>(verifier, VT_PREV_LOG_TERM, 8) &&
           VerifyField<int64_t>(verifier, VT_LEADER_COMMIT_IDX, 8) &&
           VerifyOffset(verifier, VT_ENTRIES) &&
           verifier.VerifyVector(entries()) &&
           verifier.VerifyVectorOfTables(entries()) &&
           verifier.EndTable();
  }
};

struct AppendEntryRequestBuilder {
  typedef AppendEntryRequest Table;
  ::flatbuffers::FlatBufferBuilder &fbb_;
  ::flatbuffers::uoffset_t start_;
  void add_from_id(int64_t from_id) {
    fbb_.AddElement<int64_t>(AppendEntryRequest::VT_FROM_ID, from_id, -1LL);
  }
  void add_group_id(int64_t group_id) {
    fbb_.AddElement<int64_t>(AppendEntryRequest::VT_GROUP_ID, group_id, -1LL);
  }
  void add_current_term(int64_t current_term) {
    fbb_.AddElement<int64_t>(AppendEntryRequest::VT_CURRENT_TERM, current_term, -1LL);
  }
  void add_prev_log_idx(int64_t prev_log_idx) {
    fbb_.AddElement<int64_t>(AppendEntryRequest::VT_PREV_LOG_IDX, prev_log_idx, -1LL);
  }
  void add_prev_log_term(int64_t prev_log_term) {
    fbb_.AddElement<int64_t>(AppendEntryRequest::VT_PREV_LOG_TERM, prev_log_term, -1LL);
  }
  void add_leader_commit_idx(int64_t leader_commit_idx) {
    fbb_.AddElement<int64_t>(AppendEntryRequest::VT_LEADER_COMMIT_IDX, leader_commit_idx, -1LL);
  }
  void add_entries(::flatbuffers::Offset<::flatbuffers::Vector<::flatbuffers::Offset<karma_rpc::LogEntry>>> entries) {
    fbb_.AddOffset(AppendEntryRequest::VT_ENTRIES, entries);
  }
  explicit AppendEntryRequestBuilder(::flatbuffers::FlatBufferBuilder &_fbb)
        : fbb_(_fbb) {
    start_ = fbb_.StartTable();
  }
  ::flatbuffers::Offset<AppendEntryRequest> Finish() {
    const auto end = fbb_.EndTable(start_);
    auto o = ::flatbuffers::Offset<AppendEntryRequest>(end);
    return o;
  }
};

inline ::flatbuffers::Offset<AppendEntryRequest> CreateAppendEntryRequest(
    ::flatbuffers::FlatBufferBuilder &_fbb,
    int64_t from_id = -1LL,
    int64_t group_id = -1LL,
    int64_t current_term = -1LL,
    int64_t prev_log_idx = -1LL,
    int64_t prev_log_term = -1LL,
    int64_t leader_commit_idx = -1LL,
    ::flatbuffers::Offset<::flatbuffers::Vector<::flatbuffers::Offset<karma_rpc::LogEntry>>> entries = 0) {
  AppendEntryRequestBuilder builder_(_fbb);
  builder_.add_leader_commit_idx(leader_commit_idx);
  builder_.add_prev_log_term(prev_log_term);
  builder_.add_prev_log_idx(prev_log_idx);
  builder_.add_current_term(current_term);
  builder_.add_group_id(group_id);
  builder_.add_from_id(from_id);
  builder_.add_entries(entries);
  return builder_.Finish();
}

inline ::flatbuffers::Offset<AppendEntryRequest> CreateAppendEntryRequestDirect(
    ::flatbuffers::FlatBufferBuilder &_fbb,
    int64_t from_id = -1LL,
    int64_t group_id = -1LL,
    int64_t current_term = -1LL,
    int64_t prev_log_idx = -1LL,
    int64_t prev_log_term = -1LL,
    int64_t leader_commit_idx = -1LL,
    const std::vector<::flatbuffers::Offset<karma_rpc::LogEntry>> *entries = nullptr) {
  auto entries__ = entries ? _fbb.CreateVector<::flatbuffers::Offset<karma_rpc::LogEntry>>(*entries) : 0;
  return karma_rpc::CreateAppendEntryRequest(
      _fbb,
      from_id,
      group_id,
      current_term,
      prev_log_idx,
      prev_log_term,
      leader_commit_idx,
      entries__);
}

struct LogEntry FLATBUFFERS_FINAL_CLASS : private ::flatbuffers::Table {
  typedef LogEntryBuilder Builder;
  enum FlatBuffersVTableOffset FLATBUFFERS_VTABLE_UNDERLYING_TYPE {
    VT_TERM = 4,
    VT_INDEX = 6,
    VT_COMMAND = 8
  };
  int64_t term() const {
    return GetField<int64_t>(VT_TERM, -1LL);
  }
  int64_t index() const {
    return GetField<int64_t>(VT_INDEX, -1LL);
  }
  const ::flatbuffers::String *command() const {
    return GetPointer<const ::flatbuffers::String *>(VT_COMMAND);
  }
  bool Verify(::flatbuffers::Verifier &verifier) const {
    return VerifyTableStart(verifier) &&
           VerifyField<int64_t>(verifier, VT_TERM, 8) &&
           VerifyField<int64_t>(verifier, VT_INDEX, 8) &&
           VerifyOffset(verifier, VT_COMMAND) &&
           verifier.VerifyString(command()) &&
           verifier.EndTable();
  }
};

struct LogEntryBuilder {
  typedef LogEntry Table;
  ::flatbuffers::FlatBufferBuilder &fbb_;
  ::flatbuffers::uoffset_t start_;
  void add_term(int64_t term) {
    fbb_.AddElement<int64_t>(LogEntry::VT_TERM, term, -1LL);
  }
  void add_index(int64_t index) {
    fbb_.AddElement<int64_t>(LogEntry::VT_INDEX, index, -1LL);
  }
  void add_command(::flatbuffers::Offset<::flatbuffers::String> command) {
    fbb_.AddOffset(LogEntry::VT_COMMAND, command);
  }
  explicit LogEntryBuilder(::flatbuffers::FlatBufferBuilder &_fbb)
        : fbb_(_fbb) {
    start_ = fbb_.StartTable();
  }
  ::flatbuffers::Offset<LogEntry> Finish() {
    const auto end = fbb_.EndTable(start_);
    auto o = ::flatbuffers::Offset<LogEntry>(end);
    return o;
  }
};

inline ::flatbuffers::Offset<LogEntry> CreateLogEntry(
    ::flatbuffers::FlatBufferBuilder &_fbb,
    int64_t term = -1LL,
    int64_t index = -1LL,
    ::flatbuffers::Offset<::flatbuffers::String> command = 0) {
  LogEntryBuilder builder_(_fbb);
  builder_.add_index(index);
  builder_.add_term(term);
  builder_.add_command(command);
  return builder_.Finish();
}

inline ::flatbuffers::Offset<LogEntry> CreateLogEntryDirect(
    ::flatbuffers::FlatBufferBuilder &_fbb,
    int64_t term = -1LL,
    int64_t index = -1LL,
    const char *command = nullptr) {
  auto command__ = command ? _fbb.CreateString(command) : 0;
  return karma_rpc::CreateLogEntry(
      _fbb,
      term,
      index,
      command__);
}

struct AppendEntryRejected FLATBUFFERS_FINAL_CLASS : private ::flatbuffers::Table {
  typedef AppendEntryRejectedBuilder Builder;
  enum FlatBuffersVTableOffset FLATBUFFERS_VTABLE_UNDERLYING_TYPE {
    VT_NON_MATCHING_IDX = 4,
    VT_LAST_IDX = 6
  };
  int64_t non_matching_idx() const {
    return GetField<int64_t>(VT_NON_MATCHING_IDX, -1LL);
  }
  int64_t last_idx() const {
    return GetField<int64_t>(VT_LAST_IDX, -1LL);
  }
  bool Verify(::flatbuffers::Verifier &verifier) const {
    return VerifyTableStart(verifier) &&
           VerifyField<int64_t>(verifier, VT_NON_MATCHING_IDX, 8) &&
           VerifyField<int64_t>(verifier, VT_LAST_IDX, 8) &&
           verifier.EndTable();
  }
};

struct AppendEntryRejectedBuilder {
  typedef AppendEntryRejected Table;
  ::flatbuffers::FlatBufferBuilder &fbb_;
  ::flatbuffers::uoffset_t start_;
  void add_non_matching_idx(int64_t non_matching_idx) {
    fbb_.AddElement<int64_t>(AppendEntryRejected::VT_NON_MATCHING_IDX, non_matching_idx, -1LL);
  }
  void add_last_idx(int64_t last_idx) {
    fbb_.AddElement<int64_t>(AppendEntryRejected::VT_LAST_IDX, last_idx, -1LL);
  }
  explicit AppendEntryRejectedBuilder(::flatbuffers::FlatBufferBuilder &_fbb)
        : fbb_(_fbb) {
    start_ = fbb_.StartTable();
  }
  ::flatbuffers::Offset<AppendEntryRejected> Finish() {
    const auto end = fbb_.EndTable(start_);
    auto o = ::flatbuffers::Offset<AppendEntryRejected>(end);
    return o;
  }
};

inline ::flatbuffers::Offset<AppendEntryRejected> CreateAppendEntryRejected(
    ::flatbuffers::FlatBufferBuilder &_fbb,
    int64_t non_matching_idx = -1LL,
    int64_t last_idx = -1LL) {
  AppendEntryRejectedBuilder builder_(_fbb);
  builder_.add_last_idx(last_idx);
  builder_.add_non_matching_idx(non_matching_idx);
  return builder_.Finish();
}

struct AppendEntryAccepted FLATBUFFERS_FINAL_CLASS : private ::flatbuffers::Table {
  typedef AppendEntryAcceptedBuilder Builder;
  enum FlatBuffersVTableOffset FLATBUFFERS_VTABLE_UNDERLYING_TYPE {
    VT_LAST_NEW_IDX = 4
  };
  int64_t last_new_idx() const {
    return GetField<int64_t>(VT_LAST_NEW_IDX, -1LL);
  }
  bool Verify(::flatbuffers::Verifier &verifier) const {
    return VerifyTableStart(verifier) &&
           VerifyField<int64_t>(verifier, VT_LAST_NEW_IDX, 8) &&
           verifier.EndTable();
  }
};

struct AppendEntryAcceptedBuilder {
  typedef AppendEntryAccepted Table;
  ::flatbuffers::FlatBufferBuilder &fbb_;
  ::flatbuffers::uoffset_t start_;
  void add_last_new_idx(int64_t last_new_idx) {
    fbb_.AddElement<int64_t>(AppendEntryAccepted::VT_LAST_NEW_IDX, last_new_idx, -1LL);
  }
  explicit AppendEntryAcceptedBuilder(::flatbuffers::FlatBufferBuilder &_fbb)
        : fbb_(_fbb) {
    start_ = fbb_.StartTable();
  }
  ::flatbuffers::Offset<AppendEntryAccepted> Finish() {
    const auto end = fbb_.EndTable(start_);
    auto o = ::flatbuffers::Offset<AppendEntryAccepted>(end);
    return o;
  }
};

inline ::flatbuffers::Offset<AppendEntryAccepted> CreateAppendEntryAccepted(
    ::flatbuffers::FlatBufferBuilder &_fbb,
    int64_t last_new_idx = -1LL) {
  AppendEntryAcceptedBuilder builder_(_fbb);
  builder_.add_last_new_idx(last_new_idx);
  return builder_.Finish();
}

struct AppendEntryReply FLATBUFFERS_FINAL_CLASS : private ::flatbuffers::Table {
  typedef AppendEntryReplyBuilder Builder;
  enum FlatBuffersVTableOffset FLATBUFFERS_VTABLE_UNDERLYING_TYPE {
    VT_FROM_ID = 4,
    VT_GROUP_ID = 6,
    VT_TERM = 8,
    VT_INDEX = 10,
    VT_RESULT_TYPE = 12,
    VT_RESULT = 14
  };
  int64_t from_id() const {
    return GetField<int64_t>(VT_FROM_ID, -1LL);
  }
  int64_t group_id() const {
    return GetField<int64_t>(VT_GROUP_ID, -1LL);
  }
  int64_t term() const {
    return GetField<int64_t>(VT_TERM, -1LL);
  }
  int64_t index() const {
    return GetField<int64_t>(VT_INDEX, -1LL);
  }
  karma_rpc::AppendEntryResult result_type() const {
    return static_cast<karma_rpc::AppendEntryResult>(GetField<uint8_t>(VT_RESULT_TYPE, 0));
  }
  const void *result() const {
    return GetPointer<const void *>(VT_RESULT);
  }
  template<typename T> const T *result_as() const;
  const karma_rpc::AppendEntryAccepted *result_as_AppendEntryAccepted() const {
    return result_type() == karma_rpc::AppendEntryResult_AppendEntryAccepted ? static_cast<const karma_rpc::AppendEntryAccepted *>(result()) : nullptr;
  }
  const karma_rpc::AppendEntryRejected *result_as_AppendEntryRejected() const {
    return result_type() == karma_rpc::AppendEntryResult_AppendEntryRejected ? static_cast<const karma_rpc::AppendEntryRejected *>(result()) : nullptr;
  }
  bool Verify(::flatbuffers::Verifier &verifier) const {
    return VerifyTableStart(verifier) &&
           VerifyField<int64_t>(verifier, VT_FROM_ID, 8) &&
           VerifyField<int64_t>(verifier, VT_GROUP_ID, 8) &&
           VerifyField<int64_t>(verifier, VT_TERM, 8) &&
           VerifyField<int64_t>(verifier, VT_INDEX, 8) &&
           VerifyField<uint8_t>(verifier, VT_RESULT_TYPE, 1) &&
           VerifyOffset(verifier, VT_RESULT) &&
           VerifyAppendEntryResult(verifier, result(), result_type()) &&
           verifier.EndTable();
  }
};

template<> inline const karma_rpc::AppendEntryAccepted *AppendEntryReply::result_as<karma_rpc::AppendEntryAccepted>() const {
  return result_as_AppendEntryAccepted();
}

template<> inline const karma_rpc::AppendEntryRejected *AppendEntryReply::result_as<karma_rpc::AppendEntryRejected>() const {
  return result_as_AppendEntryRejected();
}

struct AppendEntryReplyBuilder {
  typedef AppendEntryReply Table;
  ::flatbuffers::FlatBufferBuilder &fbb_;
  ::flatbuffers::uoffset_t start_;
  void add_from_id(int64_t from_id) {
    fbb_.AddElement<int64_t>(AppendEntryReply::VT_FROM_ID, from_id, -1LL);
  }
  void add_group_id(int64_t group_id) {
    fbb_.AddElement<int64_t>(AppendEntryReply::VT_GROUP_ID, group_id, -1LL);
  }
  void add_term(int64_t term) {
    fbb_.AddElement<int64_t>(AppendEntryReply::VT_TERM, term, -1LL);
  }
  void add_index(int64_t index) {
    fbb_.AddElement<int64_t>(AppendEntryReply::VT_INDEX, index, -1LL);
  }
  void add_result_type(karma_rpc::AppendEntryResult result_type) {
    fbb_.AddElement<uint8_t>(AppendEntryReply::VT_RESULT_TYPE, static_cast<uint8_t>(result_type), 0);
  }
  void add_result(::flatbuffers::Offset<void> result) {
    fbb_.AddOffset(AppendEntryReply::VT_RESULT, result);
  }
  explicit AppendEntryReplyBuilder(::flatbuffers::FlatBufferBuilder &_fbb)
        : fbb_(_fbb) {
    start_ = fbb_.StartTable();
  }
  ::flatbuffers::Offset<AppendEntryReply> Finish() {
    const auto end = fbb_.EndTable(start_);
    auto o = ::flatbuffers::Offset<AppendEntryReply>(end);
    return o;
  }
};

inline ::flatbuffers::Offset<AppendEntryReply> CreateAppendEntryReply(
    ::flatbuffers::FlatBufferBuilder &_fbb,
    int64_t from_id = -1LL,
    int64_t group_id = -1LL,
    int64_t term = -1LL,
    int64_t index = -1LL,
    karma_rpc::AppendEntryResult result_type = karma_rpc::AppendEntryResult_NONE,
    ::flatbuffers::Offset<void> result = 0) {
  AppendEntryReplyBuilder builder_(_fbb);
  builder_.add_index(index);
  builder_.add_term(term);
  builder_.add_group_id(group_id);
  builder_.add_from_id(from_id);
  builder_.add_result(result);
  builder_.add_result_type(result_type);
  return builder_.Finish();
}

struct VoteRequest FLATBUFFERS_FINAL_CLASS : private ::flatbuffers::Table {
  typedef VoteRequestBuilder Builder;
  enum FlatBuffersVTableOffset FLATBUFFERS_VTABLE_UNDERLYING_TYPE {
    VT_FROM_ID = 4,
    VT_GROUP_ID = 6,
    VT_CURRENT_TERM = 8,
    VT_LAST_LOG_IDX = 10,
    VT_LAST_LOG_TERM = 12
  };
  int64_t from_id() const {
    return GetField<int64_t>(VT_FROM_ID, -1LL);
  }
  int64_t group_id() const {
    return GetField<int64_t>(VT_GROUP_ID, -1LL);
  }
  int64_t current_term() const {
    return GetField<int64_t>(VT_CURRENT_TERM, -1LL);
  }
  int64_t last_log_idx() const {
    return GetField<int64_t>(VT_LAST_LOG_IDX, -1LL);
  }
  int64_t last_log_term() const {
    return GetField<int64_t>(VT_LAST_LOG_TERM, -1LL);
  }
  bool Verify(::flatbuffers::Verifier &verifier) const {
    return VerifyTableStart(verifier) &&
           VerifyField<int64_t>(verifier, VT_FROM_ID, 8) &&
           VerifyField<int64_t>(verifier, VT_GROUP_ID, 8) &&
           VerifyField<int64_t>(verifier, VT_CURRENT_TERM, 8) &&
           VerifyField<int64_t>(verifier, VT_LAST_LOG_IDX, 8) &&
           VerifyField<int64_t>(verifier, VT_LAST_LOG_TERM, 8) &&
           verifier.EndTable();
  }
};

struct VoteRequestBuilder {
  typedef VoteRequest Table;
  ::flatbuffers::FlatBufferBuilder &fbb_;
  ::flatbuffers::uoffset_t start_;
  void add_from_id(int64_t from_id) {
    fbb_.AddElement<int64_t>(VoteRequest::VT_FROM_ID, from_id, -1LL);
  }
  void add_group_id(int64_t group_id) {
    fbb_.AddElement<int64_t>(VoteRequest::VT_GROUP_ID, group_id, -1LL);
  }
  void add_current_term(int64_t current_term) {
    fbb_.AddElement<int64_t>(VoteRequest::VT_CURRENT_TERM, current_term, -1LL);
  }
  void add_last_log_idx(int64_t last_log_idx) {
    fbb_.AddElement<int64_t>(VoteRequest::VT_LAST_LOG_IDX, last_log_idx, -1LL);
  }
  void add_last_log_term(int64_t last_log_term) {
    fbb_.AddElement<int64_t>(VoteRequest::VT_LAST_LOG_TERM, last_log_term, -1LL);
  }
  explicit VoteRequestBuilder(::flatbuffers::FlatBufferBuilder &_fbb)
        : fbb_(_fbb) {
    start_ = fbb_.StartTable();
  }
  ::flatbuffers::Offset<VoteRequest> Finish() {
    const auto end = fbb_.EndTable(start_);
    auto o = ::flatbuffers::Offset<VoteRequest>(end);
    return o;
  }
};

inline ::flatbuffers::Offset<VoteRequest> CreateVoteRequest(
    ::flatbuffers::FlatBufferBuilder &_fbb,
    int64_t from_id = -1LL,
    int64_t group_id = -1LL,
    int64_t current_term = -1LL,
    int64_t last_log_idx = -1LL,
    int64_t last_log_term = -1LL) {
  VoteRequestBuilder builder_(_fbb);
  builder_.add_last_log_term(last_log_term);
  builder_.add_last_log_idx(last_log_idx);
  builder_.add_current_term(current_term);
  builder_.add_group_id(group_id);
  builder_.add_from_id(from_id);
  return builder_.Finish();
}

struct VoteReply FLATBUFFERS_FINAL_CLASS : private ::flatbuffers::Table {
  typedef VoteReplyBuilder Builder;
  enum FlatBuffersVTableOffset FLATBUFFERS_VTABLE_UNDERLYING_TYPE {
    VT_FROM_ID = 4,
    VT_GROUP_ID = 6,
    VT_CURRENT_TERM = 8,
    VT_VOTE_GRANTED = 10
  };
  int64_t from_id() const {
    return GetField<int64_t>(VT_FROM_ID, -1LL);
  }
  int64_t group_id() const {
    return GetField<int64_t>(VT_GROUP_ID, -1LL);
  }
  int64_t current_term() const {
    return GetField<int64_t>(VT_CURRENT_TERM, -1LL);
  }
  int64_t vote_granted() const {
    return GetField<int64_t>(VT_VOTE_GRANTED, -1LL);
  }
  bool Verify(::flatbuffers::Verifier &verifier) const {
    return VerifyTableStart(verifier) &&
           VerifyField<int64_t>(verifier, VT_FROM_ID, 8) &&
           VerifyField<int64_t>(verifier, VT_GROUP_ID, 8) &&
           VerifyField<int64_t>(verifier, VT_CURRENT_TERM, 8) &&
           VerifyField<int64_t>(verifier, VT_VOTE_GRANTED, 8) &&
           verifier.EndTable();
  }
};

struct VoteReplyBuilder {
  typedef VoteReply Table;
  ::flatbuffers::FlatBufferBuilder &fbb_;
  ::flatbuffers::uoffset_t start_;
  void add_from_id(int64_t from_id) {
    fbb_.AddElement<int64_t>(VoteReply::VT_FROM_ID, from_id, -1LL);
  }
  void add_group_id(int64_t group_id) {
    fbb_.AddElement<int64_t>(VoteReply::VT_GROUP_ID, group_id, -1LL);
  }
  void add_current_term(int64_t current_term) {
    fbb_.AddElement<int64_t>(VoteReply::VT_CURRENT_TERM, current_term, -1LL);
  }
  void add_vote_granted(int64_t vote_granted) {
    fbb_.AddElement<int64_t>(VoteReply::VT_VOTE_GRANTED, vote_granted, -1LL);
  }
  explicit VoteReplyBuilder(::flatbuffers::FlatBufferBuilder &_fbb)
        : fbb_(_fbb) {
    start_ = fbb_.StartTable();
  }
  ::flatbuffers::Offset<VoteReply> Finish() {
    const auto end = fbb_.EndTable(start_);
    auto o = ::flatbuffers::Offset<VoteReply>(end);
    return o;
  }
};

inline ::flatbuffers::Offset<VoteReply> CreateVoteReply(
    ::flatbuffers::FlatBufferBuilder &_fbb,
    int64_t from_id = -1LL,
    int64_t group_id = -1LL,
    int64_t current_term = -1LL,
    int64_t vote_granted = -1LL) {
  VoteReplyBuilder builder_(_fbb);
  builder_.add_vote_granted(vote_granted);
  builder_.add_current_term(current_term);
  builder_.add_group_id(group_id);
  builder_.add_from_id(from_id);
  return builder_.Finish();
}

struct TimeOut FLATBUFFERS_FINAL_CLASS : private ::flatbuffers::Table {
  typedef TimeOutBuilder Builder;
  enum FlatBuffersVTableOffset FLATBUFFERS_VTABLE_UNDERLYING_TYPE {
    VT_FROM_ID = 4,
    VT_GROUP_ID = 6,
    VT_CURRENT_TERM = 8
  };
  int64_t from_id() const {
    return GetField<int64_t>(VT_FROM_ID, -1LL);
  }
  int64_t group_id() const {
    return GetField<int64_t>(VT_GROUP_ID, -1LL);
  }
  int64_t current_term() const {
    return GetField<int64_t>(VT_CURRENT_TERM, -1LL);
  }
  bool Verify(::flatbuffers::Verifier &verifier) const {
    return VerifyTableStart(verifier) &&
           VerifyField<int64_t>(verifier, VT_FROM_ID, 8) &&
           VerifyField<int64_t>(verifier, VT_GROUP_ID, 8) &&
           VerifyField<int64_t>(verifier, VT_CURRENT_TERM, 8) &&
           verifier.EndTable();
  }
};

struct TimeOutBuilder {
  typedef TimeOut Table;
  ::flatbuffers::FlatBufferBuilder &fbb_;
  ::flatbuffers::uoffset_t start_;
  void add_from_id(int64_t from_id) {
    fbb_.AddElement<int64_t>(TimeOut::VT_FROM_ID, from_id, -1LL);
  }
  void add_group_id(int64_t group_id) {
    fbb_.AddElement<int64_t>(TimeOut::VT_GROUP_ID, group_id, -1LL);
  }
  void add_current_term(int64_t current_term) {
    fbb_.AddElement<int64_t>(TimeOut::VT_CURRENT_TERM, current_term, -1LL);
  }
  explicit TimeOutBuilder(::flatbuffers::FlatBufferBuilder &_fbb)
        : fbb_(_fbb) {
    start_ = fbb_.StartTable();
  }
  ::flatbuffers::Offset<TimeOut> Finish() {
    const auto end = fbb_.EndTable(start_);
    auto o = ::flatbuffers::Offset<TimeOut>(end);
    return o;
  }
};

inline ::flatbuffers::Offset<TimeOut> CreateTimeOut(
    ::flatbuffers::FlatBufferBuilder &_fbb,
    int64_t from_id = -1LL,
    int64_t group_id = -1LL,
    int64_t current_term = -1LL) {
  TimeOutBuilder builder_(_fbb);
  builder_.add_current_term(current_term);
  builder_.add_group_id(group_id);
  builder_.add_from_id(from_id);
  return builder_.Finish();
}

struct ReadQuorum FLATBUFFERS_FINAL_CLASS : private ::flatbuffers::Table {
  typedef ReadQuorumBuilder Builder;
  enum FlatBuffersVTableOffset FLATBUFFERS_VTABLE_UNDERLYING_TYPE {
    VT_FROM_ID = 4,
    VT_GROUP_ID = 6,
    VT_CURRENT_TERM = 8,
    VT_LEADER_COMMIT_IDX = 10,
    VT_ID = 12
  };
  int64_t from_id() const {
    return GetField<int64_t>(VT_FROM_ID, -1LL);
  }
  int64_t group_id() const {
    return GetField<int64_t>(VT_GROUP_ID, -1LL);
  }
  int64_t current_term() const {
    return GetField<int64_t>(VT_CURRENT_TERM, -1LL);
  }
  int64_t leader_commit_idx() const {
    return GetField<int64_t>(VT_LEADER_COMMIT_IDX, -1LL);
  }
  int64_t id() const {
    return GetField<int64_t>(VT_ID, -1LL);
  }
  bool Verify(::flatbuffers::Verifier &verifier) const {
    return VerifyTableStart(verifier) &&
           VerifyField<int64_t>(verifier, VT_FROM_ID, 8) &&
           VerifyField<int64_t>(verifier, VT_GROUP_ID, 8) &&
           VerifyField<int64_t>(verifier, VT_CURRENT_TERM, 8) &&
           VerifyField<int64_t>(verifier, VT_LEADER_COMMIT_IDX, 8) &&
           VerifyField<int64_t>(verifier, VT_ID, 8) &&
           verifier.EndTable();
  }
};

struct ReadQuorumBuilder {
  typedef ReadQuorum Table;
  ::flatbuffers::FlatBufferBuilder &fbb_;
  ::flatbuffers::uoffset_t start_;
  void add_from_id(int64_t from_id) {
    fbb_.AddElement<int64_t>(ReadQuorum::VT_FROM_ID, from_id, -1LL);
  }
  void add_group_id(int64_t group_id) {
    fbb_.AddElement<int64_t>(ReadQuorum::VT_GROUP_ID, group_id, -1LL);
  }
  void add_current_term(int64_t current_term) {
    fbb_.AddElement<int64_t>(ReadQuorum::VT_CURRENT_TERM, current_term, -1LL);
  }
  void add_leader_commit_idx(int64_t leader_commit_idx) {
    fbb_.AddElement<int64_t>(ReadQuorum::VT_LEADER_COMMIT_IDX, leader_commit_idx, -1LL);
  }
  void add_id(int64_t id) {
    fbb_.AddElement<int64_t>(ReadQuorum::VT_ID, id, -1LL);
  }
  explicit ReadQuorumBuilder(::flatbuffers::FlatBufferBuilder &_fbb)
        : fbb_(_fbb) {
    start_ = fbb_.StartTable();
  }
  ::flatbuffers::Offset<ReadQuorum> Finish() {
    const auto end = fbb_.EndTable(start_);
    auto o = ::flatbuffers::Offset<ReadQuorum>(end);
    return o;
  }
};

inline ::flatbuffers::Offset<ReadQuorum> CreateReadQuorum(
    ::flatbuffers::FlatBufferBuilder &_fbb,
    int64_t from_id = -1LL,
    int64_t group_id = -1LL,
    int64_t current_term = -1LL,
    int64_t leader_commit_idx = -1LL,
    int64_t id = -1LL) {
  ReadQuorumBuilder builder_(_fbb);
  builder_.add_id(id);
  builder_.add_leader_commit_idx(leader_commit_idx);
  builder_.add_current_term(current_term);
  builder_.add_group_id(group_id);
  builder_.add_from_id(from_id);
  return builder_.Finish();
}

struct ReadQuorumReply FLATBUFFERS_FINAL_CLASS : private ::flatbuffers::Table {
  typedef ReadQuorumReplyBuilder Builder;
  enum FlatBuffersVTableOffset FLATBUFFERS_VTABLE_UNDERLYING_TYPE {
    VT_FROM_ID = 4,
    VT_GROUP_ID = 6,
    VT_CURRENT_TERM = 8,
    VT_COMMIT_IDX = 10,
    VT_ID = 12
  };
  int64_t from_id() const {
    return GetField<int64_t>(VT_FROM_ID, -1LL);
  }
  int64_t group_id() const {
    return GetField<int64_t>(VT_GROUP_ID, -1LL);
  }
  int64_t current_term() const {
    return GetField<int64_t>(VT_CURRENT_TERM, -1LL);
  }
  int64_t commit_idx() const {
    return GetField<int64_t>(VT_COMMIT_IDX, -1LL);
  }
  int64_t id() const {
    return GetField<int64_t>(VT_ID, -1LL);
  }
  bool Verify(::flatbuffers::Verifier &verifier) const {
    return VerifyTableStart(verifier) &&
           VerifyField<int64_t>(verifier, VT_FROM_ID, 8) &&
           VerifyField<int64_t>(verifier, VT_GROUP_ID, 8) &&
           VerifyField<int64_t>(verifier, VT_CURRENT_TERM, 8) &&
           VerifyField<int64_t>(verifier, VT_COMMIT_IDX, 8) &&
           VerifyField<int64_t>(verifier, VT_ID, 8) &&
           verifier.EndTable();
  }
};

struct ReadQuorumReplyBuilder {
  typedef ReadQuorumReply Table;
  ::flatbuffers::FlatBufferBuilder &fbb_;
  ::flatbuffers::uoffset_t start_;
  void add_from_id(int64_t from_id) {
    fbb_.AddElement<int64_t>(ReadQuorumReply::VT_FROM_ID, from_id, -1LL);
  }
  void add_group_id(int64_t group_id) {
    fbb_.AddElement<int64_t>(ReadQuorumReply::VT_GROUP_ID, group_id, -1LL);
  }
  void add_current_term(int64_t current_term) {
    fbb_.AddElement<int64_t>(ReadQuorumReply::VT_CURRENT_TERM, current_term, -1LL);
  }
  void add_commit_idx(int64_t commit_idx) {
    fbb_.AddElement<int64_t>(ReadQuorumReply::VT_COMMIT_IDX, commit_idx, -1LL);
  }
  void add_id(int64_t id) {
    fbb_.AddElement<int64_t>(ReadQuorumReply::VT_ID, id, -1LL);
  }
  explicit ReadQuorumReplyBuilder(::flatbuffers::FlatBufferBuilder &_fbb)
        : fbb_(_fbb) {
    start_ = fbb_.StartTable();
  }
  ::flatbuffers::Offset<ReadQuorumReply> Finish() {
    const auto end = fbb_.EndTable(start_);
    auto o = ::flatbuffers::Offset<ReadQuorumReply>(end);
    return o;
  }
};

inline ::flatbuffers::Offset<ReadQuorumReply> CreateReadQuorumReply(
    ::flatbuffers::FlatBufferBuilder &_fbb,
    int64_t from_id = -1LL,
    int64_t group_id = -1LL,
    int64_t current_term = -1LL,
    int64_t commit_idx = -1LL,
    int64_t id = -1LL) {
  ReadQuorumReplyBuilder builder_(_fbb);
  builder_.add_id(id);
  builder_.add_commit_idx(commit_idx);
  builder_.add_current_term(current_term);
  builder_.add_group_id(group_id);
  builder_.add_from_id(from_id);
  return builder_.Finish();
}

struct EchoRequest FLATBUFFERS_FINAL_CLASS : private ::flatbuffers::Table {
  typedef EchoRequestBuilder Builder;
  enum FlatBuffersVTableOffset FLATBUFFERS_VTABLE_UNDERLYING_TYPE {
    VT_FROM_ID = 4,
    VT_GROUP_ID = 6,
    VT_MSG = 8
  };
  int64_t from_id() const {
    return GetField<int64_t>(VT_FROM_ID, -1LL);
  }
  int64_t group_id() const {
    return GetField<int64_t>(VT_GROUP_ID, -1LL);
  }
  const ::flatbuffers::String *msg() const {
    return GetPointer<const ::flatbuffers::String *>(VT_MSG);
  }
  bool Verify(::flatbuffers::Verifier &verifier) const {
    return VerifyTableStart(verifier) &&
           VerifyField<int64_t>(verifier, VT_FROM_ID, 8) &&
           VerifyField<int64_t>(verifier, VT_GROUP_ID, 8) &&
           VerifyOffset(verifier, VT_MSG) &&
           verifier.VerifyString(msg()) &&
           verifier.EndTable();
  }
};

struct EchoRequestBuilder {
  typedef EchoRequest Table;
  ::flatbuffers::FlatBufferBuilder &fbb_;
  ::flatbuffers::uoffset_t start_;
  void add_from_id(int64_t from_id) {
    fbb_.AddElement<int64_t>(EchoRequest::VT_FROM_ID, from_id, -1LL);
  }
  void add_group_id(int64_t group_id) {
    fbb_.AddElement<int64_t>(EchoRequest::VT_GROUP_ID, group_id, -1LL);
  }
  void add_msg(::flatbuffers::Offset<::flatbuffers::String> msg) {
    fbb_.AddOffset(EchoRequest::VT_MSG, msg);
  }
  explicit EchoRequestBuilder(::flatbuffers::FlatBufferBuilder &_fbb)
        : fbb_(_fbb) {
    start_ = fbb_.StartTable();
  }
  ::flatbuffers::Offset<EchoRequest> Finish() {
    const auto end = fbb_.EndTable(start_);
    auto o = ::flatbuffers::Offset<EchoRequest>(end);
    return o;
  }
};

inline ::flatbuffers::Offset<EchoRequest> CreateEchoRequest(
    ::flatbuffers::FlatBufferBuilder &_fbb,
    int64_t from_id = -1LL,
    int64_t group_id = -1LL,
    ::flatbuffers::Offset<::flatbuffers::String> msg = 0) {
  EchoRequestBuilder builder_(_fbb);
  builder_.add_group_id(group_id);
  builder_.add_from_id(from_id);
  builder_.add_msg(msg);
  return builder_.Finish();
}

inline ::flatbuffers::Offset<EchoRequest> CreateEchoRequestDirect(
    ::flatbuffers::FlatBufferBuilder &_fbb,
    int64_t from_id = -1LL,
    int64_t group_id = -1LL,
    const char *msg = nullptr) {
  auto msg__ = msg ? _fbb.CreateString(msg) : 0;
  return karma_rpc::CreateEchoRequest(
      _fbb,
      from_id,
      group_id,
      msg__);
}

struct EchoReply FLATBUFFERS_FINAL_CLASS : private ::flatbuffers::Table {
  typedef EchoReplyBuilder Builder;
  enum FlatBuffersVTableOffset FLATBUFFERS_VTABLE_UNDERLYING_TYPE {
    VT_FROM_ID = 4,
    VT_GROUP_ID = 6,
    VT_MSG = 8
  };
  int64_t from_id() const {
    return GetField<int64_t>(VT_FROM_ID, -1LL);
  }
  int64_t group_id() const {
    return GetField<int64_t>(VT_GROUP_ID, -1LL);
  }
  const ::flatbuffers::String *msg() const {
    return GetPointer<const ::flatbuffers::String *>(VT_MSG);
  }
  bool Verify(::flatbuffers::Verifier &verifier) const {
    return VerifyTableStart(verifier) &&
           VerifyField<int64_t>(verifier, VT_FROM_ID, 8) &&
           VerifyField<int64_t>(verifier, VT_GROUP_ID, 8) &&
           VerifyOffset(verifier, VT_MSG) &&
           verifier.VerifyString(msg()) &&
           verifier.EndTable();
  }
};

struct EchoReplyBuilder {
  typedef EchoReply Table;
  ::flatbuffers::FlatBufferBuilder &fbb_;
  ::flatbuffers::uoffset_t start_;
  void add_from_id(int64_t from_id) {
    fbb_.AddElement<int64_t>(EchoReply::VT_FROM_ID, from_id, -1LL);
  }
  void add_group_id(int64_t group_id) {
    fbb_.AddElement<int64_t>(EchoReply::VT_GROUP_ID, group_id, -1LL);
  }
  void add_msg(::flatbuffers::Offset<::flatbuffers::String> msg) {
    fbb_.AddOffset(EchoReply::VT_MSG, msg);
  }
  explicit EchoReplyBuilder(::flatbuffers::FlatBufferBuilder &_fbb)
        : fbb_(_fbb) {
    start_ = fbb_.StartTable();
  }
  ::flatbuffers::Offset<EchoReply> Finish() {
    const auto end = fbb_.EndTable(start_);
    auto o = ::flatbuffers::Offset<EchoReply>(end);
    return o;
  }
};

inline ::flatbuffers::Offset<EchoReply> CreateEchoReply(
    ::flatbuffers::FlatBufferBuilder &_fbb,
    int64_t from_id = -1LL,
    int64_t group_id = -1LL,
    ::flatbuffers::Offset<::flatbuffers::String> msg = 0) {
  EchoReplyBuilder builder_(_fbb);
  builder_.add_group_id(group_id);
  builder_.add_from_id(from_id);
  builder_.add_msg(msg);
  return builder_.Finish();
}

inline ::flatbuffers::Offset<EchoReply> CreateEchoReplyDirect(
    ::flatbuffers::FlatBufferBuilder &_fbb,
    int64_t from_id = -1LL,
    int64_t group_id = -1LL,
    const char *msg = nullptr) {
  auto msg__ = msg ? _fbb.CreateString(msg) : 0;
  return karma_rpc::CreateEchoReply(
      _fbb,
      from_id,
      group_id,
      msg__);
}

inline bool VerifyAppendEntryResult(::flatbuffers::Verifier &verifier, const void *obj, AppendEntryResult type) {
  switch (type) {
    case AppendEntryResult_NONE: {
      return true;
    }
    case AppendEntryResult_AppendEntryAccepted: {
      auto ptr = reinterpret_cast<const karma_rpc::AppendEntryAccepted *>(obj);
      return verifier.VerifyTable(ptr);
    }
    case AppendEntryResult_AppendEntryRejected: {
      auto ptr = reinterpret_cast<const karma_rpc::AppendEntryRejected *>(obj);
      return verifier.VerifyTable(ptr);
    }
    default: return true;
  }
}

inline bool VerifyAppendEntryResultVector(::flatbuffers::Verifier &verifier, const ::flatbuffers::Vector<::flatbuffers::Offset<void>> *values, const ::flatbuffers::Vector<uint8_t> *types) {
  if (!values || !types) return !values && !types;
  if (values->size() != types->size()) return false;
  for (::flatbuffers::uoffset_t i = 0; i < values->size(); ++i) {
    if (!VerifyAppendEntryResult(
        verifier,  values->Get(i), types->GetEnum<AppendEntryResult>(i))) {
      return false;
    }
  }
  return true;
}

}  // namespace karma_rpc

#endif  // FLATBUFFERS_GENERATED_RPC_KARMA_RPC_H_
