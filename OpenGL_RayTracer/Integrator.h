#pragma once
#include <functional>
#include <iostream>

namespace integrator {

template <class T>
struct State {
    T x {}, v {};
};

template <class T>
struct Derivative {
    T dx {}, dv {};
};

template <class T>
Derivative<T> operator*(float scale, const Derivative<T>& d) { return { d.dx * scale, d.dv * scale }; }
template <class T>
Derivative<T> operator*(const Derivative<T>& d, float scale) { return { d.dx * scale, d.dv * scale }; }

template <class T, class Data>
struct RK4 {
    template <class T, class Data>
    Derivative<T> evaluate(const State<T>& initial, const Derivative<T>& d, const Data& data, float dt)
    {
        State<T> state;
        state.x = initial.x + d.dx * dt;
        state.v = initial.v + d.dv * dt;

        Derivative<T> output;
        output.dx = state.v;
        output.dv = accelerationFunction(state, data);

        return output;
    }

    template <class T, class Data>
    void integrate(State<T>& state, const Data& data, float dt)
    {

        if (increaseIntCountThreshold) {
            int nextIterCount = increaseIntCountThreshold(accelerationFunction(state, data));
            if (nextIterCount >= iterCount)
                iterCount = nextIterCount;
            else
                iterCount /= 2;

            if (iterCount > maxIterCount) {
                iterCount = maxIterCount;
                state.v = T {};
            }
            iterCount = std::max(iterCount, 1);
        }
        float interStep = 1.f / iterCount;
        Derivative<T> a, b, c, d;

        Derivative<T> init = {};
        for (int i = 0; i < iterCount; ++i) {
            if (evaluationCallBack)
                evaluationCallBack(state);
            a = interStep * evaluate(state, init, data, 0.0f);
            b = interStep * evaluate(state, a, data, dt * 0.5f);
            c = interStep * evaluate(state, b, data, dt * 0.5f);
            d = interStep * evaluate(state, c, data, dt);
            init = d;

            T dxdt = 1.0f / 6.0f * (a.dx + 2.0f * (b.dx + c.dx) + d.dx);
            T dvdt = 1.0f / 6.0f * (a.dv + 2.0f * (b.dv + c.dv) + d.dv);

            state.x = state.x + dxdt * dt;
            state.v = state.v + dvdt * dt;
        }
    }

    std::function<T(const State<T>&, const Data&)> accelerationFunction;
    std::function<int(const T&)> increaseIntCountThreshold;
    std::function<void(const State<T>&)> evaluationCallBack;
    int iterCount = 1;
    int maxIterCount = 100;
};

}
