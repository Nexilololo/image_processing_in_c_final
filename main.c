#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "bmp8.h"
#include "bmp24.h"

float **create_kernel(int size, const float *values) {
    if (size <= 0 || size % 2 == 0) {
        fprintf(stderr, "Kernel size must be positive and odd.\n");
        return NULL;
    }
    float **kernel = (float **)malloc(size * sizeof(float *));
    if (!kernel) {
        fprintf(stderr, "Failed to allocate kernel rows.\n");
        return NULL;
    }
    for (int i = 0; i < size; i++) {
        kernel[i] = (float *)malloc(size * sizeof(float));
        if (!kernel[i]) {
            fprintf(stderr, "Failed to allocate kernel col for row %d.\n", i);
            for (int j = 0; j < i; j++) free(kernel[j]);
            free(kernel);
            return NULL;
        }
        if (values) {
            for (int j = 0; j < size; j++) {
                kernel[i][j] = values[i * size + j];
            }
        }
    }
    return kernel;
}

void free_kernel(float **kernel, int size) {
    if (!kernel || size <= 0) return;
    for (int i = 0; i < size; i++) {
        free(kernel[i]);
    }
    free(kernel);
}

// Kernel Definitions
const float box_blur_values_3x3[] = {
    1.0f/9.0f, 1.0f/9.0f, 1.0f/9.0f,
    1.0f/9.0f, 1.0f/9.0f, 1.0f/9.0f,
    1.0f/9.0f, 1.0f/9.0f, 1.0f/9.0f
};

// Gaussian Blur (3x3)
const float gaussian_blur_values_3x3[] = {
    1.0f/16.0f, 2.0f/16.0f, 1.0f/16.0f,
    2.0f/16.0f, 4.0f/16.0f, 2.0f/16.0f,
    1.0f/16.0f, 2.0f/16.0f, 1.0f/16.0f
};

// Outline (3x3)
const float outline_values_3x3[] = {
    -1.0f, -1.0f, -1.0f,
    -1.0f,  8.0f, -1.0f,
    -1.0f, -1.0f, -1.0f
};

// Emboss (3x3)
const float emboss_values_3x3[] = {
    -2.0f, -1.0f,  0.0f,
    -1.0f,  1.0f,  1.0f,
     0.0f,  1.0f,  2.0f
};

// Sharpen (3x3)
const float sharpen_values_3x3[] = {
     0.0f, -1.0f,  0.0f,
    -1.0f,  5.0f, -1.0f,
     0.0f, -1.0f,  0.0f
};


// Menu Functions
void display_main_menu() {
    printf("\n--- Image Processing Main Menu ---\n");
    printf("1. Process 8-bit Grayscale Image (BMP8)\n2. Process 24-bit Color Image (BMP24)\n3. Exit\n");
    printf(">>> Your choice: ");
}

void display_operation_menu(const char* image_type) {
    printf("\n--- %s Image Operations ---\n", image_type);
    printf("1. Open an image\n2. Save an image\n3. Apply a filter\n4. Display image information\n5. Return to Main Menu\n");
    printf(">>> Your choice: ");
}

void display_filter_menu_bmp8() {
    printf("\n--- BMP8 Filters/Operations ---\n1. Negative\n2. Brightness\n3. Threshold\n4. Box Blur\n5. Gaussian Blur\n6. Outline\n7. Emboss\n8. Sharpen\n9. Histogram Equalization\n10. Return to BMP8 Menu\n");
    printf(">>> Your choice: ");
}

void display_filter_menu_bmp24() {
    printf("\n--- BMP24 Filters/Operations ---\n1. Negative\n2. Grayscale\n3. Brightness\n4. Box Blur\n5. Gaussian Blur\n6. Outline\n7. Emboss\n8. Sharpen\n9. Histogram Equalization\n10. Return to BMP24 Menu\n");
    printf(">>> Your choice: ");
}

int get_int_input() {
    int value;
    char buffer[100];
    if (fgets(buffer, sizeof(buffer), stdin)) {
        if (sscanf(buffer, "%d", &value) == 1) {
            return value;
        }
    }
    printf("Invalid input. Please enter an integer.\n");
    return -1; // Indicate error
}

void get_string_input(const char* prompt, char* buffer, int size) {
    printf("%s", prompt);
    if (fgets(buffer, size, stdin)) {
        // To remove trailing newline if present
        buffer[strcspn(buffer, "\n")] = 0;
    } else {
        buffer[0] = '\0'; // To sure there aren't null-terminated on error
    }
}


void process_bmp8_menu() {
    t_bmp8 *img8 = NULL;
    char filename[256];
    int choice;

    float **kernel_box = create_kernel(3, box_blur_values_3x3);
    float **kernel_gaussian = create_kernel(3, gaussian_blur_values_3x3);
    float **kernel_outline = create_kernel(3, outline_values_3x3);
    float **kernel_emboss = create_kernel(3, emboss_values_3x3);
    float **kernel_sharpen = create_kernel(3, sharpen_values_3x3);


    do {
        display_operation_menu("8-bit Grayscale (BMP8)");
        choice = get_int_input("");

        switch (choice) {
            case 1: // Open image
                if (img8) bmp8_free(img8);
                get_string_input("File path: ", filename, sizeof(filename));
                img8 = bmp8_loadImage(filename);
                if (img8) printf("Image loaded successfully!\n");
                else printf("Failed to load image.\n");
                break;
            case 2: // Save image
                if (!img8) {
                    printf("No image loaded to save.\n");
                    break;
                }
                get_string_input("Save as file path: ", filename, sizeof(filename));
                bmp8_saveImage(filename, img8);
                printf("Image saved successfully (or attempt made)!\n");
                break;
            case 3: // Apply filter
                if (!img8) {
                    printf("No image loaded to apply filter.\n");
                    break;
                }
                int filter_choice;
                display_filter_menu_bmp8();
                filter_choice = get_int_input("");
                switch (filter_choice) {
                    case 1: bmp8_negative(img8); printf("Negative filter applied.\n"); break;
                    case 2: {
                        int bright_val = get_int_input("Enter brightness value (-255 to 255): ");
                        if (bright_val != -1) bmp8_brightness(img8, bright_val);
                        printf("Brightness adjusted.\n");
                        break;
                    }
                    case 3: {
                        int thresh_val = get_int_input("Enter threshold value (0 to 255): ");
                        if (thresh_val != -1) bmp8_threshold(img8, thresh_val);
                        printf("Threshold applied.\n");
                        break;
                    }
                    case 4: if(kernel_box) bmp8_applyFilter(img8, kernel_box, 3); printf("Box Blur applied.\n"); break;
                    case 5: if(kernel_gaussian) bmp8_applyFilter(img8, kernel_gaussian, 3); printf("Gaussian Blur applied.\n"); break;
                    case 6: if(kernel_outline) bmp8_applyFilter(img8, kernel_outline, 3); printf("Outline filter applied.\n"); break;
                    case 7: if(kernel_emboss) bmp8_applyFilter(img8, kernel_emboss, 3); printf("Emboss filter applied.\n"); break;
                    case 8: if(kernel_sharpen) bmp8_applyFilter(img8, kernel_sharpen, 3); printf("Sharpen filter applied.\n"); break;
                    case 9: {
                        unsigned int *hist = bmp8_computeHistogram(img8);
                        if (hist) {
                            unsigned int *cdf_map = bmp8_computeCDF(hist);
                            if (cdf_map) {
                                bmp8_equalize(img8, cdf_map);
                                printf("Histogram equalization applied.\n");
                                free(cdf_map);
                            } else {
                                printf("Failed to compute CDF map for equalization.\n");
                            }
                            free(hist);
                        } else {
                            printf("Failed to compute histogram for equalization.\n");
                        }
                        break;
                    }
                    case 10: printf("Returning to BMP8 menu.\n"); break;
                    default: printf("Invalid filter choice.\n"); break;
                }
                break;
            case 4: // Display info
                if (img8) bmp8_printInfo(img8);
                else printf("No image loaded.\n");
                break;
            case 5: // Return to main menu
                printf("Returning to Main Menu...\n");
                break;
            default:
                printf("Invalid choice. Please try again.\n");
                break;
        }
    } while (choice != 5);

    if (img8) bmp8_free(img8);
    free_kernel(kernel_box, 3);
    free_kernel(kernel_gaussian, 3);
    free_kernel(kernel_outline, 3);
    free_kernel(kernel_emboss, 3);
    free_kernel(kernel_sharpen, 3);
}


void process_bmp24_menu() {
    t_bmp24 *img24 = NULL;
    char filename[256];
    int choice;

    float **kernel_box = create_kernel(3, box_blur_values_3x3);
    float **kernel_gaussian = create_kernel(3, gaussian_blur_values_3x3);
    float **kernel_outline = create_kernel(3, outline_values_3x3);
    float **kernel_emboss = create_kernel(3, emboss_values_3x3);
    float **kernel_sharpen = create_kernel(3, sharpen_values_3x3);

    // Temporary image for convolution results
    t_bmp24 *temp_img_for_conv = NULL;

    do {
        display_operation_menu("24-bit Color (BMP24)");
        choice = get_int_input("");

        switch (choice) {
            case 1: // Open image
                if (img24) bmp24_free(img24);
                if (temp_img_for_conv) bmp24_free(temp_img_for_conv); temp_img_for_conv = NULL;

                get_string_input("File path: ", filename, sizeof(filename));
                img24 = bmp24_loadImage(filename);
                if (img24) {
                    printf("Image loaded successfully!\n");
                    // Allocate temp image for convolution
                    temp_img_for_conv = bmp24_allocate(img24->width, img24->height, img24->colorDepth);
                    if (!temp_img_for_conv) {
                        printf("Warning: Could not allocate temporary image for convolution.\n");
                    } else {
                        temp_img_for_conv->header = img24->header;
                        temp_img_for_conv->header_info = img24->header_info;
                    }
                } else {
                    printf("Failed to load image.\n");
                }
                break;
            case 2: // Save image
                if (!img24) {
                    printf("No image loaded to save.\n");
                    break;
                }
                get_string_input("Save as file path: ", filename, sizeof(filename));
                bmp24_saveImage(img24, filename);
                printf("Image saved successfully (or attempt made)!\n");
                break;
            case 3: // Apply filter/operation
                if (!img24) {
                    printf("No image loaded to apply filter.\n");
                    break;
                }
                int filter_choice;
                display_filter_menu_bmp24();
                filter_choice = get_int_input("");
                switch (filter_choice) {
                    case 1: bmp24_negative(img24); printf("Negative filter applied.\n"); break;
                    case 2: bmp24_grayscale(img24); printf("Grayscale conversion applied.\n"); break;
                    case 3: {
                        int bright_val = get_int_input("Enter brightness value (-255 to 255): ");
                        if (bright_val != -1) bmp24_brightness(img24, bright_val);
                        printf("Brightness adjusted.\n");
                        break;
                    }
                    case 4: // Box Blur
                    case 5: // Gaussian Blur
                    case 6: // Outline
                    case 7: // Emboss
                    case 8: // Sharpen
                        {
                            float **selected_kernel = NULL;
                            const char* filter_name = "";
                            if (filter_choice == 4) { selected_kernel = kernel_box; filter_name = "Box Blur"; }
                            else if (filter_choice == 5) { selected_kernel = kernel_gaussian; filter_name = "Gaussian Blur"; }
                            else if (filter_choice == 6) { selected_kernel = kernel_outline; filter_name = "Outline"; }
                            else if (filter_choice == 7) { selected_kernel = kernel_emboss; filter_name = "Emboss"; }
                            else if (filter_choice == 8) { selected_kernel = kernel_sharpen; filter_name = "Sharpen"; }

                            if (selected_kernel && temp_img_for_conv) {
                                // Apply convolution
                                int n = 3 / 2; // Kernel half-size for 3x3
                                for (int y = n; y < img24->height - n; y++) {
                                    for (int x = n; x < img24->width - n; x++) {
                                        temp_img_for_conv->data[y][x] = bmp24_convolution(img24, x, y, selected_kernel, 3);
                                    }
                                }
                                for (int y = n; y < img24->height - n; y++) {
                                    for (int x = n; x < img24->width - n; x++) {
                                        img24->data[y][x] = temp_img_for_conv->data[y][x];
                                    }
                                }
                                printf("%s filter applied.\n", filter_name);
                            } else {
                                printf("Kernel or temporary image not available for convolution.\n");
                            }
                        }
                        break;
                    case 9: // Histogram Equalization
                        bmp24_equalize(img24);
                        printf("Histogram equalization (Y component) applied.\n");
                        break;
                    case 10: printf("Returning to BMP24 menu.\n"); break;
                    default: printf("Invalid filter choice.\n"); break;
                }
                break;
            case 4: // Display info
                if (img24) {
                    printf("Image Info (BMP24):\n");
                    printf("  Width: %d\n", img24->width);
                    printf("  Height: %d\n", img24->height);
                    printf("  Color Depth: %d\n", img24->colorDepth);
                    printf("  File Size (from header): %u bytes\n", img24->header.size);
                    printf("  Image Data Offset (from header): %u\n", img24->header.offset);
                    printf("  Image Data Size (from header_info): %u bytes\n", img24->header_info.imagesize);
                } else {
                    printf("No image loaded.\n");
                }
                break;
            case 5: // Return to main menu
                printf("Returning to Main Menu...\n");
                break;
            default:
                printf("Invalid choice. Please try again.\n");
                break;
        }
    } while (choice != 5);

    if (img24) bmp24_free(img24);
    if (temp_img_for_conv) bmp24_free(temp_img_for_conv);

    free_kernel(kernel_box, 3);
    free_kernel(kernel_gaussian, 3);
    free_kernel(kernel_outline, 3);
    free_kernel(kernel_emboss, 3);
    free_kernel(kernel_sharpen, 3);
}


int main() {
    int main_choice;
    do {
        display_main_menu();
        main_choice = get_int_input();

        switch (main_choice) {
            case 1:
                process_bmp8_menu();
                break;
            case 2:
                process_bmp24_menu();
                break;
            case 3:
                printf("Exiting program.\n");
                break;
            default:
                printf("Invalid choice. Please try again.\n");
                break;
        }
    } while (main_choice != 3);

    return 0;
}