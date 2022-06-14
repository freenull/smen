#ifndef SMEN_MATH_HPP
#define SMEN_MATH_HPP

#include <string>
#include <sstream>

namespace smen {
    template <typename T>
    struct Size;

    using Size2 = Size<int>;
    using FloatSize2 = Size<float>;

    template <typename T>
    struct Vector {
        using ElementType = T;

        ElementType x;
        ElementType y;

        Vector(ElementType x, ElementType y) {
            this->x = x;
            this->y = y;
        }

        inline void write_string(std::ostream & s) const {
            s << typeid(T).name() << "(" << x << ", " << y << ")";
        }

        inline const std::string to_string() const {
            auto s = std::stringstream();
            write_string(s);
            return s.str();
        }

        inline const Size<ElementType> to_size() const;
    };

    template <typename T>
    inline std::ostream & operator<<(std::ostream & s, const Vector<T> & vec) {
        vec.write_string(s);
        return s;
    }

    template <typename T>
    inline Vector<T> && operator +(const Vector<T> & left, const Vector<T> right) {
        return Vector<T>(left.x + right.x, left.y + right.y);
    }

    template <typename T>
    inline Vector<T> && operator -(const Vector<T> & left, const Vector<T> right) {
        return Vector<T>(left.x - right.x, left.y - right.y);
    }

    using Vector2 = Vector<int>;
    using FloatVector2 = Vector<float>;

    template <typename T>
    struct Motion {
        using ElementType = T;

        ElementType x;
        ElementType y;
        ElementType rel_x;
        ElementType rel_y;

        inline ElementType new_x() const { return x + rel_x; }
        inline ElementType new_y() const { return y + rel_y; }

        Motion(ElementType x, ElementType y, ElementType rel_x, ElementType rel_y) {
            this->x = x;
            this->y = y;
            this->rel_x = rel_x;
            this->rel_y = rel_y;
        }

        inline void write_string(std::ostream & s) const {
            s << typeid(T).name() << "(" << x << ", " << y << " -> " << new_x() << ", " << new_y() << ")";
        }

        inline const std::string to_string() const {
            auto s = std::stringstream();
            write_string(s);
            return s.str();
        }

        inline const Size<ElementType> old_to_size() const;
        inline const Size<ElementType> new_to_size() const;
    };
    
    using Motion2 = Motion<int>;
    using FloatMotion2 = Motion<float>;

    template <typename T>
    inline std::ostream & operator<<(std::ostream & s, const Motion<T> & motion) {
        motion.write_string(s);
        return s;
    }

    template <typename T>
    struct Size {
        using ElementType = T;

        ElementType w;
        ElementType h;

        Size(ElementType w, ElementType h) {
            this->w = w;
            this->h = h;
        }

        inline void write_string(std::ostream & s) const {
            s << typeid(T).name() << "(" << w << ", " << h << ")";
        }

        const std::string to_string() const {
            auto s = std::stringstream();
            write_string(s);
            return s.str();
        }

        const Vector<ElementType> to_vector() const;
    };


    template <typename T>
    inline std::ostream & operator<<(std::ostream & s, const Size<T> & size) {
        size.write_string(s);
        return s;
    }

    template <typename T>
    inline Vector<T> && operator *(const Vector<T> & left, const Size<T> right) {
        return Vector<T>(left.x * right.w, left.y * right.h);
    }

    template <typename T>
    inline Vector<T> && operator *(const Size<T> & left, const Vector<T> right) {
        return Vector<T>(left.x * right.w, left.y * right.h);
    }

    template <typename T>
    inline Vector<T> && operator /(const Vector<T> & left, const Size<T> right) {
        return Vector<T>(left.x / right.w, left.y / right.h);
    }

    template <typename T>
    inline Size<T> && operator +(const Size<T> & left, const Size<T> right) {
        return Vector<T>(left.w + right.w, left.h + right.h);
    }

    template <typename T>
    inline Size<T> && operator -(const Size<T> & left, const Size<T> right) {
        return Vector<T>(left.w - right.w, left.h - right.h);
    }

    struct Rect {
        using ElementType = int;

        ElementType x;
        ElementType y;
        ElementType w;
        ElementType h;

        inline const Vector2 pos() const {
            return Vector2(x, y);
        }

        inline void set_pos(Vector2 pos) {
            this->x = pos.x;
            this->y = pos.y;
        }

        inline const Size2 size() const {
            return Size2(w, h);
        }

        inline void set_size(Size2 size) {
            this->w = size.w;
            this->h = size.h;
        }

        Rect(const Vector2 & pos, const Size2 & size) {
            this->x = pos.x;
            this->y = pos.y;
            this->w = size.w;
            this->h = size.h;
        }

        Rect(ElementType x, ElementType y, ElementType w, ElementType h) {
            this->x = x;
            this->y = y;
            this->w = w;
            this->h = h;
        }

        inline void write_string(std::ostream & s) const {
            s << "Rect([" << x << ", " << y << "], [" << w << ", " << h << "])";
        }

        const std::string to_string() const {
            auto s = std::stringstream();
            write_string(s);
            return s.str();
        }
    };

    inline std::ostream & operator<<(std::ostream & s, const Rect & rect) {
        rect.write_string(s);
        return s;
    }
}

#endif//SMEN_MATH_HPP
