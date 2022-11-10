/******************************************************************************
 * @file                    GPIOs.hpp
 * @author                  Rafael Andrioli Bauer
 * @date                    09.11.2022
 * @matriculation number    5163344
 * @e-mail contact          abauer.rafael@gmail.com
 *
 * @brief   Header contains abstraction of GPIOs
 *
 * Description: In order to abstract the manipulation of registers and
 *              ease up maintainability of the application code, this header
 *              provides an easy interface to set specific GPIOs as inputs or
 *              outputs. It allows the user to hold a handle of an specific pin
 *              which makes easier to manipulate and even listen to the pin
 *              events.
 *
 *              The interface was developed using template classes as well as
 *              constexpr so the program memory usage would be optimized.
 ******************************************************************************/
#ifndef COMMON_GPIOS_HPP_
#define COMMON_GPIOS_HPP_

#include "helpers.hpp"

#include <msp430g2553.h>
#include <cstdint>

namespace Microtech {

/**
 * This is a actually serves as a namespace to keep some GPIO classes together
 * and hides away some non-public types.
 */
class GPIOs {
public:
    /**
     * Enum to differentiate between the different IO Ports available
     * in the Microcontroller. This type is used in some template arguments
     */
    enum class Port {
        PORT_1 = 1,     ///< Represents the port 1
        PORT_2,         ///< Represents the port 2
        PORT_3,         ///< Represents the port 3
    };

    /**
     * Enum representing the different IO states
     */
    enum class IOState {
        LOW = 0,        ///< The IO is low.
        HIGH,           ///< The IO is high.
    };
private:
    /**
     * Helper class to return the references of the
     * GPIO registers.
     * Since the name always follows a pattern, such as
     * "PxDIR" where "x" would be replaced by the port number, this was
     * created, so one can retrieve the correct port
     * register references only by calling the respective
     * method.
     *
     * This class also helps to prevent bugs from being introduced, since
     * one simply declares the constexpr object and then uses the "Px" register
     * methods. This way, one doesn't forget to swap one value when doing
     * a change.
     *
     * The port number can be specified as a template parameter.
     *
     * @tparam port Port in which one wants to get the register references from
     */
    template <Port port>
    class Registers {
    public:
        constexpr Registers() {};

        /**
         * Returns the PxDir register reference.
         * Since this is a static constexpr function,
         * the compiler already resolves the internal logic.
         * There is no extra allocation in program memory
         *
         * @returns The reference to the respective PxDir register
         */
        static constexpr volatile uint8_t& PxDir() {
            switch (port) {
           case Port::PORT_1: return P1DIR;
           case Port::PORT_2: return P2DIR;
           default:
           case Port::PORT_3: return P3DIR;
           }
        }
        /**
         * Returns the PxSel register reference.
         * Since this is a static constexpr function,
         * the compiler already resolves the internal logic.
         * There is no extra allocation in program memory
         *
         * @returns The reference to the respective PxSel register
         */
        static constexpr volatile uint8_t& PxSel() {
            switch (port) {
            case Port::PORT_1: return P1SEL;
            case Port::PORT_2: return P2SEL;
            default:
            case Port::PORT_3: return P3SEL;
            }
        }
        /**
         * Returns the PxSel2 register reference.
         * Since this is a static constexpr function,
         * the compiler already resolves the internal logic.
         * There is no extra allocation in program memory
         *
         * @returns The reference to the respective PxSel2 register
         */
        static constexpr volatile uint8_t& PxSel2() {
            switch (port) {
            case Port::PORT_1: return P1SEL2;
            case Port::PORT_2: return P2SEL2;
            default:
            case Port::PORT_3: return P3SEL2;
            }
        }
        /**
         * Returns the PxRen register reference.
         * Since this is a static constexpr function,
         * the compiler already resolves the internal logic.
         * There is no extra allocation in program memory.
         *
         * @returns The reference to the respective PxRen register
         */
        static constexpr volatile uint8_t& PxRen() {
            switch (port) {
            case Port::PORT_1: return P1REN;
            case Port::PORT_2: return P2REN;
            default:
            case Port::PORT_3: return P3REN;
            }
        }
        /**
         * Returns the PxIn register reference.
         * Since this is a static constexpr function,
         * the compiler already resolves the internal logic.
         * There is no extra allocation in program memory.
         *
         * @returns The reference to the respective PxIn register
         */
        static constexpr volatile uint8_t& PxIn() {
            switch (port) {
            case Port::PORT_1: return P1IN;
            case Port::PORT_2: return P2IN;
            default:
            case Port::PORT_3: return P3IN;
            }
        }
        /**
         * Returns the PxOut register reference.
         * Since this is a static constexpr function,
         * the compiler already resolves the internal logic.
         * There is no extra allocation in program memory.
         *
         * @returns The reference to the respective PxOut register
         */
        static constexpr volatile uint8_t& PxOut() noexcept {
            switch (port) {
            case Port::PORT_1: return P1OUT;
            case Port::PORT_2: return P2OUT;
            default:
            case Port::PORT_3: return P3OUT;
            }
        }
    };

    /**
     * Class serves as a base class and holds the common attributes between
     * OutputHandle and InputHandle.
     *
     * It cannot be constructed by anyone else other then its childs, since the constructor
     * is protected.
     *
     * @tparam port Port of the pin in which the handle will be for
     */
    template <Port port>
    class IoHandleBase {
    public:
        /**
         * Gets the state of the Pin.
         *
         * The compiler tries to already resolve this in compile time.
         *
         * @returns The state of the pin. IOState::HIGH or IOState::LOW
         */
        constexpr IOState getState() noexcept {
            // According to MSP430 Manual, PxIn will always be updated with the pins state
            // regardless if it is configured as an input or output.
            if(getRegisterBits(registers.PxIn(), bitMask, (uint8_t)0)) {
                return IOState::HIGH;
            } else {
                return IOState::LOW;
            }
        }
    protected:
        constexpr IoHandleBase(const uint8_t desiredBitMask) : bitMask(desiredBitMask) {}

        const uint8_t bitMask;                          ///< The mask of the pin that this handle takes care of
        static constexpr Registers<port> registers{};   ///< The registers of the port
    };

public:
    /**
     * Class is a public interface to use an IO pin.
     * It serves as a handle to an output pin.
     * Whenever someone creates the handle, the GPIO pin gets set as an output.
     * So to use a pin as an output, is very straight forward. One only has to call:
     *  @code
     *      GPIOs::OutputHandle<GPIOs::Port::PORT_1> p1_0 = GPIOs::getOutputHandle<GPIOs::Port::PORT_1>(static_cast<uint8_t>(BIT0));
     *      p1_0.setState(GPIOs::IOState::HIGH);
     *      p1_0.setState(GPIOs::IOState::LOW);
     *  @endcode
     *
     * @tparam port Port of the pin in which the handle will be for
     */
    template <Port port>
    class OutputHandle : public IoHandleBase<port>{
    public:
        /**
         * Class constructor. It is responsible for calling the parent class constructor
         * as well as setting this IO as an output.
         *
         * Since the constructor is constexpr the compiler tries to resolved in compile time
         */
        constexpr OutputHandle(const uint8_t bitMask) : IoHandleBase<port>(bitMask) {
            setRegisterBits(IoHandleBase<port>::registers.PxDir(), bitMask);
            resetRegisterBits(IoHandleBase<port>::registers.PxSel(), bitMask);
            resetRegisterBits(IoHandleBase<port>::registers.PxSel2(), bitMask);
        }

        /**
         * Sets the Pin to an specific state. IOState::HIGH or IOState::LOW
         *
         * The compiler tries to already resolve this in compile time.
         *
         * @param state Desired pin state.
         */
        constexpr void setState(IOState state) {
            if(state == IOState::HIGH) {
                setRegisterBits(IoHandleBase<port>::registers.PxOut(), IoHandleBase<port>::bitMask);
            } else {
                resetRegisterBits(IoHandleBase<port>::registers.PxOut(), IoHandleBase<port>::bitMask);
            }
        }

        /**
         * Sets the Pin to an specific state. IOState::HIGH or IOState::LOW
         * Overloaded from before so it can also accept boolean as an input
         * The compiler tries to already resolve this in compile time.
         *
         * @param state Desired pin state.
         */
        constexpr void setState(bool state) {
            if(state) {
                setRegisterBits(IoHandleBase<port>::registers.PxOut(), IoHandleBase<port>::bitMask);
            } else {
                resetRegisterBits(IoHandleBase<port>::registers.PxOut(), IoHandleBase<port>::bitMask);
            }
        }


        /**
         * Toggles the Pin. If the pin state is IOState::HIGH it will toggle to IOState::LOW or vice-versa
         *
         * The compiler tries to already resolve this in compile time.
         *
         */
        constexpr void toggle() {
            toggleRegisterBits(IoHandleBase<port>::registers.PxOut(), IoHandleBase<port>::bitMask);
        }
    };

    /**
     * Class is a public interface to use an IO pin.
     * It serves as a handle to an input pin.
     * Whenever someone creates the handle, the GPIO pin gets set as an input.
     * So to use a pin as an input, is very straight forward. One only has to call:
     *  @code
     *      GPIOs::InputHandle<GPIOs::Port::PORT_1> p1_0 = GPIOs::getInputHandle<GPIOs::Port::PORT_1>(static_cast<uint8_t>(BIT0));
     *      GPIOs::IOState stateP1_0 = p1_0.getState();
     *  @endcode
     *
     * @tparam port Port of the pin in which the handle will be for
     */
    template <Port port>
    class InputHandle : public IoHandleBase<port>{
    public:
        /**
         * Class constructor. It is responsible for calling the parent class constructor
         * as well as setting this IO as an input.
         *
         * Since the constructor is constexpr the compiler tries to resolved in compile time
         *
         * @note At the moment the input is always enabling the internal pull-up resistor.
         */
        constexpr InputHandle(const uint8_t bitMask) : IoHandleBase<port>(bitMask) {
            resetRegisterBits(IoHandleBase<port>::registers.PxDir(), bitMask);
            resetRegisterBits(IoHandleBase<port>::registers.PxSel(), bitMask);
            resetRegisterBits(IoHandleBase<port>::registers.PxSel2(), bitMask);
            // Enabled Pull-Up resistor!
            setRegisterBits(IoHandleBase<port>::registers.PxOut(), bitMask);
            setRegisterBits(IoHandleBase<port>::registers.PxRen(), bitMask);
        }

        // getState class is implemented in the IoHandleBase, since this class inherits from it
        // it also has that functionality
    };

    /**
     * Constructor of the GPIOs class
     */
    constexpr GPIOs() {};

    /**
     * Method to initialize the GPIOs and put them in a known state.
     * Usually this will be called only once
     */
    constexpr void initGpios() noexcept {
        constexpr Registers<Port::PORT_1> registers;
        // Initialize all  outputs of port 1 to 0
        resetRegisterBits<uint8_t>(registers.PxOut(), static_cast<uint8_t>(0xFF));
        // Disable all internal resistors of the port
        resetRegisterBits(registers.PxRen(), static_cast<uint8_t>(0xFF));
    }

    /**
     * Method to get an OutputHandle. To see the purpose of an OutputHandle, please check its documentation
     * above.
     *
     * @tparam port The port that the OutputHandle should be create for.
     */
    template <Port port>
    static constexpr OutputHandle<port> getOutputHandle(const uint8_t bitSelection) noexcept {
        return OutputHandle<port>(bitSelection);
    }

    /**
     * Method to get an InputHandle. To see the purpose of an InputHandle, please check its documentation
     * above.
     *
     * @tparam port The port that the InputHandle should be create for.
     */
    template <Port port>
    static constexpr InputHandle<port> getInputHandle(const uint8_t bitSelection) noexcept {
        return InputHandle<port>(bitSelection);
    }

};
} /* namespace Microtech */
#endif /* COMMON_GPIOS_HPP_ */