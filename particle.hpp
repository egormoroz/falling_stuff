#ifndef PARTICLE_HPP
#define PARTICLE_HPP

#include <cstdint>

enum class ParticleType : uint8_t {
    None = 0,
    Sand,
    Water,
    Wood,
    Fire,
};

struct None {
    static const ParticleType TYPE = ParticleType::None;
};

struct Sand {
    static const ParticleType TYPE = ParticleType::Sand;

    uint16_t vy;
};

struct Water {
    static const ParticleType TYPE = ParticleType::Water;

    int8_t flow_dir;
};

struct Wood {
    static const ParticleType TYPE = ParticleType::Wood;
};

struct Fire {
    static const ParticleType TYPE = ParticleType::Fire;

    uint16_t lifetime; //in millis
};

class Particle {
    //bitmask
    //1..7 bits: particle type
    //8th bit: been_updated flag
    uint8_t m_data;
    //Ready to pack some other general properties here
    uint8_t padding;

    Particle(ParticleType tp, bool been_updated = false) 
        : m_data(uint8_t(tp) | uint8_t(been_updated) << 7)
    {}
public:
    Particle()
        : Particle(None::TYPE) {}

    template<typename T>
    static Particle create();

    template<typename T>
    bool is() const {
        return type() == T::TYPE;
    }

    bool been_updated() const { return m_data & (1 << 7); }

    template<bool b>
    void set_updated() {
        if (b)
            m_data |= 1 << 7;
        else
            m_data &= ~(1 << 7);
    }

    ParticleType type() const {
        return ParticleType(m_data & ~(1 << 7));
    }

public:
    union {
        None none;
        Sand sand;
        Water water;
        Wood wood;
        Fire fire;
    } as;
};

template<>
inline Particle Particle::create<None>() { return Particle(); }

template<>
inline Particle Particle::create<Sand>() {
    Particle p(Sand::TYPE);
    p.as.sand.vy = 0;
    return p;
}

template<>
inline Particle Particle::create<Water>() {
    Particle p(Water::TYPE);
    p.as.water.flow_dir = -1;
    return p;
}

template<>
inline Particle Particle::create<Wood>() {
    return Particle(Wood::TYPE);
}

template<>
inline Particle Particle::create<Fire>() {
    Particle p(Fire::TYPE);
    p.as.fire.lifetime = 4000; //4s 
    return p;
}

#endif

