/*
 * Copyright (C) 2019 Inria
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     tests
 *
 * @file
 * @brief       TensorFlow Lite MNIST MLP inference functions
 *
 * @author      Alexandre Abadie <alexandre.abadie@inria.fr>
 */

#include <stdio.h>
#include "kernel_defines.h"

#include "tensorflow/lite/micro/micro_mutable_op_resolver.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/micro/system_setup.h"

#include "tensorflow/lite/schema/schema_generated.h"

#include "blob/model.tflite.h"

// Globals, used for compatibility with Arduino-style sketches.
namespace {
    const tflite::Model* model = nullptr;
    tflite::MicroInterpreter* interpreter = nullptr;
    TfLiteTensor* input = nullptr;
    TfLiteTensor* output = nullptr;

    // Create an area of memory to use for input, output, and intermediate arrays.
    constexpr int kTensorArenaSize = 11 * 1024;
    uint8_t tensor_arena[kTensorArenaSize];
}  // namespace


// The name of this function is important for Arduino compatibility.
void setup()
{
#if IS_USED(MODULE_TFLITE_MICRO)
    tflite::InitializeTarget();
#endif

    // Map the model into a usable data structure. This doesn't involve any
    // copying or parsing, it's a very lightweight operation.
    model = tflite::GetModel(model_tflite);

    if (model->version() != TFLITE_SCHEMA_VERSION) {
        printf("Model provided is schema version %d not equal "
               "to supported version %d.\n",
               static_cast<uint8_t>(model->version()), TFLITE_SCHEMA_VERSION);
        return;
    }

    // This pulls in all the operation implementations we need.
    static tflite::MicroMutableOpResolver<12> resolver;

    // Registering all the necessary operators
    if (resolver.AddConv2D() != kTfLiteOk) {
        puts("Failed to register Conv2D");
        return;
    }
    if (resolver.AddExpandDims() != kTfLiteOk) {
        puts("Failed to register ExpandDims");
        return;
    }
    if (resolver.AddReshape() != kTfLiteOk) {
        puts("Failed to register Reshape");
        return;
    }
    if (resolver.AddRelu() != kTfLiteOk) {
        puts("Failed to register Relu");
        return;
    }
    if (resolver.AddMean() != kTfLiteOk) {
        puts("Failed to register Mean");
        return;
    }
    if (resolver.AddFullyConnected() != kTfLiteOk) {
        puts("Failed to register FullyConnected");
        return;
    }
    if (resolver.AddQuantize() != kTfLiteOk) {
        puts("Failed to register Quantize");
        return;
    }
    if (resolver.AddDequantize() != kTfLiteOk) {
        puts("Failed to register Dequantize");
        return;
    }
    if (resolver.AddSoftmax() != kTfLiteOk) {
        puts("Failed to register Softmax");
        return;
    }
    
    // Add ADD operator, which was missing
    if (resolver.AddAdd() != kTfLiteOk) {
        puts("Failed to register Add operator");
        return;
    }

    // Register the MaxPool2D operator
    if (resolver.AddMaxPool2D() != kTfLiteOk) {
        puts("Failed to register MaxPool2D");
        return;
    }
    

    // Build an interpreter to run the model with.
    static tflite::MicroInterpreter static_interpreter(
        model, resolver, tensor_arena, kTensorArenaSize);
    interpreter = &static_interpreter;

    // Allocate memory from the tensor_arena for the model's tensors.
    TfLiteStatus allocate_status = interpreter->AllocateTensors();
    if (allocate_status != kTfLiteOk) {
        puts("AllocateTensors() failed");
        return;
    }

    puts("Setup successful");
}

// The name of this function is important for Arduino compatibility.
int predict(float *imu_data, int data_len, float threshold, int class_num){
    // Copy IMU data into the input tensor
    input = interpreter->input(0);
    output = interpreter->output(0);
    
    if (input == nullptr || output == nullptr) {
        puts("Failed to get input/output tensors.");
        return -1;
    }

    for (unsigned i = 0; i < data_len; ++i) {
        input->data.f[i] = static_cast<float>(imu_data[i]);
    }

    // Run inference, and report any error
    TfLiteStatus invoke_status = interpreter->Invoke();
    if (invoke_status != kTfLiteOk) {
        puts("Invoke failed");
        return -1;
    }

    // Get the best match from the output tensor  
    float val = 0; 
    uint8_t res = 0;

    printf("------------------------------\n");
    for (unsigned i = 0; i < class_num; ++i) {
        float current = output->data.f[i];
        printf("[%d] value: %.03f\n", i, current);
        if (current > threshold && current > val) {
            val = current;
            res = i;
        }
    }

    // Output the prediction, if there's one
    if (val > 0) {
        printf("Motion prediction: %d\n", res);
    } else {
        puts("No match found");
    }

    return res;
}

