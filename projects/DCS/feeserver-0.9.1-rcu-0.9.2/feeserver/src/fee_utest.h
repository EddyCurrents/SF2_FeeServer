/************************************************************************
 * **
 * **
 * ** This file is property of and copyright by the Department of Physics
 * ** Institute for Physic and Technology, University of Bergen,
 * ** Bergen, Norway.
 * ** In cooperation with Center for Technology Transfer and 
 * ** Telecommunication (ZTT), University of Applied Sciences Worms
 * ** Worms, Germany.
 * **
 * ** This file has been written by Sebastian Bablok,
 * ** Sebastian.Bablok@uib.no
 * **
 * ** Important: This file is provided without any warranty, including
 * ** fitness for any particular purpose. Further distribution of this file,
 * ** even with changes in the code, is only allowed, when this copyright
 * ** and warranty paragraph is kept unchanged and included to the sources. 
 * **
 * **
 * *************************************************************************/

#ifndef FEE_UTEST_H
#define FEE_UTEST_H

#ifdef __UTEST

#include <stdbool.h>

/**
 * @defgroup feesrv_utest Unit tests
 * Used to test the functions of the feeserver. The funtionality can be enabled by
 * __UTEST
 *
 * @author Christian Kofler, Sebastian Bablok
 * @ingroup feesrv_core
 */

typedef bool (*TestFuncPtr)();

/**
 * This is the test-main, where every test has to be called.
 * @ingroup feesrv_utest
 */
void testFrameWork();

/**
 * This takes care of setUp and tearDown and calls the test-function.
 *
 * @param function pointer to actual testfunction, that has to be executed.
 * @ingroup feesrv_utest
 */
bool test(TestFuncPtr function);

/**
 * Sets up the preconditions.
 * @ingroup feesrv_utest
 */
void setUp();

/**
 * Does the cleanUp stuff.
 * @ingroup feesrv_utest
 */
void tearDown();

/**
 * Tests the publish() of the FEE-SERVER.
 *
 * @param runs pointer to counter of test-runs
 * @param failures pointer to counter of failures
 * @param errors pointer to counter of errors
 *
 * @return true, if all tests succeeded, else false
 * @ingroup feesrv_utest
 */
bool testPublish(int* runs, int* failures, int* errors);

/**
 * Tests the add_item_node of the doubly-linked-list.
 *
 * @param runs pointer to counter of test-runs
 * @param failures pointer to counter of failures
 * @param errors pointer to counter of errors
 *
 * @return true, if all tests succeeded, else false
 * @ingroup feesrv_utest
 */
bool testAdd_item_node(int* runs, int* failures, int* errors);

/**
 * Tests the start method, which normally starts the DIM - server
 *
 * @param runs pointer to counter of test-runs
 * @param failures pointer to counter of failures
 * @param errors pointer to counter of errors
 *
 * @return true, if all tests succeeded, else false
 * @ingroup feesrv_utest
 */
bool testStart(int* runs, int* failures, int* errors);

/**
 * Tests the function, which calculates and compares the checksum.
 *
 * @param runs pointer to counter of test-runs
 * @param failures pointer to counter of failures
 * @param errors pointer to counter of errors
 *
 * @return true, if all tests succeeded, else false
 * @ingroup feesrv_utest
 */
bool testChecksumCal(int* runs, int* failures, int* errors);

/**
 * Tests the ack_service method, normally called by the DIM-Framework to get
 * access to the result-data.
 *
 * @param runs pointer to counter of test-runs
 * @param failures pointer to counter of failures
 * @param errors pointer to counter of errors
 *
 * @return true, if all tests succeeded, else false
 * @ingroup feesrv_utest
 */
bool testAck_service(int* runs, int* failures, int* errors);

/**
 * Tests the signalCEready method, normally used to signal the CE is ready,
 * here test if signal is sent.
 *
 * @param runs pointer to counter of test-runs
 * @param failures pointer to counter of failures
 * @param errors pointer to counter of errors
 *
 * @return true, if all tests succeeded, else false
 * @ingroup feesrv_utest
 */
bool testSignalCEready(int* runs, int* failures, int* errors);

/**
 * Tests the method called checkLocation, which performs a check on the address
 * of the location of a monitored value for bitflips and tries, if possible, to
 * repair the location.
 *
 * @param runs pointer to counter of test-runs
 * @param failures pointer to counter of failures
 * @param errors pointer to counter of errors
 *
 * @return true, if all tests succeeded, else false
 * @ingroup feesrv_utest
 */
bool testCheckLocation(int* runs, int* failures, int* errors);

/**
 * Method to call the signalCEready function in an own thread.
 * @ingroup feesrv_utest
 */
void signalThread();

/**
 * Method to call the signalCEready function in an own thread.
 * @ingroup feesrv_utest
 */
void signalThreadTimeout();

#endif

#endif
