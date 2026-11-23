// Headless checks of PickRequests, the bookkeeping behind on-request mouse picking: when a request becomes active, how it
// rides with a frame's readback and how it fires. No engine, no GPU: ctest runs this.
#include "source/components/renderingManager/renderer3D/PickRequests.h"
#include <iostream>
#include <optional>
#include <vector>

namespace {

  int g_checks = 0;
  int g_failures = 0;

  void check(const bool condition,
             const char* name)
  {
    ++g_checks;

    if (!condition)
    {
      ++g_failures;
      std::cout << "FAIL: " << name << '\n';
    }
  }

  using Requests = vke::PickRequests<int>;
  using Callbacks = std::vector<Requests::Callback>;

  // One call: which request fired and what it received (-1 for nullopt).
  struct Call {
    int id;
    int value;
  };

  Requests::Callback recorder(std::vector<Call>& calls,
                              const int id)
  {
    return [&calls, id](const std::optional<int>& result) {
      calls.push_back({ id, result ? *result : -1 });
    };
  }

  void checkNotActiveUntilPromoted()
  {
    Requests requests;
    std::vector<Call> calls;

    check(!requests.hasActive(), "idle: nothing is active");

    requests.request(recorder(calls, 1));

    check(!requests.hasActive(), "a request is not active before the frame boundary");

    Callbacks slot;
    requests.take(slot);
    check(slot.empty(), "a frame taking requests before the boundary gets none");

    requests.promote();

    check(requests.hasActive(), "promoted: the request is active");

    requests.take(slot);
    check(slot.size() == 1, "the active request goes to the slot");
    check(!requests.hasActive(), "taking the requests empties the active set");
  }

  void checkFiresWithResult()
  {
    Requests requests;
    std::vector<Call> calls;
    Callbacks slot;

    requests.request(recorder(calls, 1));
    requests.request(recorder(calls, 2));
    requests.promote();
    requests.take(slot);

    check(calls.empty(), "nothing fires before the readback resolves");

    Requests::fire(slot, 7);

    check(calls.size() == 2, "both requests fired");
    check(calls.size() == 2 && calls[0].value == 7 && calls[1].value == 7, "both received the resolved result");
    check(slot.empty(), "the slot is emptied after firing");
  }

  void checkNullopt()
  {
    Requests requests;
    std::vector<Call> calls;
    Callbacks slot;

    // Empty space, or a frame that could not pick: the same nullopt reaches the callbacks
    requests.request(recorder(calls, 1));
    requests.promote();
    requests.take(slot);
    Requests::fire(slot, std::nullopt);

    check(calls.size() == 1 && calls[0].value == -1, "nullopt reaches the callback");

    requests.request(recorder(calls, 2));
    requests.promote();
    requests.take(slot);
    Requests::fire(slot, 0);

    check(calls.size() == 2 && calls[1].value == 0, "a real result of zero is not mistaken for nullopt");
  }

  void checkSeveralInOrder()
  {
    Requests requests;
    std::vector<Call> calls;
    Callbacks slot;

    for (int id = 1; id <= 5; ++id)
    {
      requests.request(recorder(calls, id));
    }

    requests.promote();
    requests.take(slot);
    Requests::fire(slot, 3);

    bool inOrder = calls.size() == 5;

    for (size_t i = 0; i < calls.size(); ++i)
    {
      inOrder = inOrder && calls[i].id == static_cast<int>(i) + 1;
    }

    check(inOrder, "several requests in one window all fire, in request order");
  }

  void checkWindowsAreSeparate()
  {
    Requests requests;
    std::vector<Call> calls;
    Callbacks firstSlot;
    Callbacks secondSlot;

    requests.request(recorder(calls, 1));
    requests.promote();

    // Made while the frame that carries request 1 is being submitted or rendered
    requests.request(recorder(calls, 2));

    requests.take(firstSlot);
    check(firstSlot.size() == 1, "a request made after the boundary does not join the frame");

    requests.promote();
    requests.take(secondSlot);
    check(secondSlot.size() == 1, "it joins the next frame");

    Requests::fire(firstSlot, 10);
    Requests::fire(secondSlot, 20);

    check(calls.size() == 2 && calls[0].id == 1 && calls[0].value == 10 && calls[1].id == 2 && calls[1].value == 20,
          "each window's requests get their own frame's result");
  }

  void checkCarryOver()
  {
    Requests requests;
    std::vector<Call> calls;
    Callbacks slot;

    requests.request(recorder(calls, 1));
    requests.promote();

    // Frames that never reach their readback end without taking anything
    requests.promote();
    requests.promote();

    check(requests.hasActive(), "a request survives frames that never reach the readback");
    check(calls.empty(), "and has not fired meanwhile");

    requests.request(recorder(calls, 2));
    requests.promote();
    requests.take(slot);
    Requests::fire(slot, 5);

    check(calls.size() == 2 && calls[0].id == 1 && calls[1].id == 2, "it fires for the next frame that does, in order");
    check(calls.size() == 2 && calls[0].value == 5, "with that frame's result");
  }

  void checkExactlyOnce()
  {
    Requests requests;
    std::vector<Call> calls;
    Callbacks slot;

    requests.request(recorder(calls, 1));
    requests.promote();
    requests.take(slot);
    Requests::fire(slot, 1);

    check(calls.size() == 1, "fired once");

    Requests::fire(slot, 2);
    requests.promote();
    requests.take(slot);
    Requests::fire(slot, 3);

    check(calls.size() == 1, "later resolves and frame boundaries never fire it again");
    check(!requests.hasActive(), "and nothing stays active");
  }

  void checkIdle()
  {
    Requests requests;
    std::vector<Call> calls;
    Callbacks slot;

    for (int frame = 0; frame < 10; ++frame)
    {
      requests.promote();
      requests.take(slot);
      Requests::fire(slot, frame);
    }

    check(calls.empty() && slot.empty() && slot.capacity() == 0 && !requests.hasActive(),
          "idle frames fire nothing and allocate no slot storage");

    // Positive control: a request does use the slot
    requests.request(recorder(calls, 1));
    requests.promote();
    requests.take(slot);

    check(slot.size() == 1 && slot.capacity() >= 1, "a request does use the slot");

    const size_t capacity = slot.capacity();
    Requests::fire(slot, 0);
    requests.promote();
    requests.take(slot);

    check(slot.empty() && slot.capacity() == capacity, "the slot keeps its capacity once fired");
  }

  void checkReentrantRequest()
  {
    Requests requests;
    std::vector<Call> calls;
    Callbacks slot;

    // A callback may ask for another pick; it waits for the next window rather than joining the one firing
    requests.request([&](const std::optional<int>&) {
      calls.push_back({ 1, 0 });
      requests.request(recorder(calls, 2));
    });
    requests.promote();
    requests.take(slot);
    Requests::fire(slot, 0);

    check(calls.size() == 1 && !requests.hasActive(), "a request made from a callback is not active yet");

    requests.promote();
    requests.take(slot);
    Requests::fire(slot, 0);

    check(calls.size() == 2 && calls[1].id == 2, "it fires for the following frame");
  }

} // namespace

int main()
{
  checkNotActiveUntilPromoted();
  checkFiresWithResult();
  checkNullopt();
  checkSeveralInOrder();
  checkWindowsAreSeparate();
  checkCarryOver();
  checkExactlyOnce();
  checkIdle();
  checkReentrantRequest();

  std::cout << g_checks << " checks, " << g_failures << " failures\n";

  return g_failures == 0 ? 0 : 1;
}
