/*
 * main.c
 * Entry point / manual test driver for the Fun language interpreter.
 *
 * This file builds a small "compiler pipeline":
 *   source text  ->  lexer (regex-based)  ->  token stream
 *                ->  parser (recursive descent) -> AST
 *                ->  interpreter (static lazy / eager modes)
 *
 * Memory note:
 * runtime objects are arena-allocated.
 */

#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>

#include "arena.h"
#include "interpret.h"
#include "lexer_regex.h"
#include "parser.h"
#include "utils_funcs.h"


#define EXAMPLES_DIRECTORY "examples"
#define EXAMPLE_EXTENSION ".fun"

static int has_example_extension( const char *name )
{
    size_t name_length = strlen( name );
    size_t extension_length = strlen( EXAMPLE_EXTENSION );

    return name_length > extension_length &&
           strcmp( name + name_length - extension_length, EXAMPLE_EXTENSION ) == 0;
}

static char *read_example_file( const char *path )
{
    FILE *file = fopen( path, "rb" );
    if ( !file )
    {
        fprintf( stderr, "Could not open example '%s': %s\n", path, strerror( errno ) );
        exit( EXIT_FAILURE );
    }

    if ( fseek( file, 0, SEEK_END ) != 0 )
    {
        fprintf( stderr, "Could not read example '%s': %s\n", path, strerror( errno ) );
        fclose( file );
        exit( EXIT_FAILURE );
    }

    long file_size = ftell( file );
    if ( file_size < 0 || fseek( file, 0, SEEK_SET ) != 0 )
    {
        fprintf( stderr, "Could not read example '%s': %s\n", path, strerror( errno ) );
        fclose( file );
        exit( EXIT_FAILURE );
    }

    char *source = malloc( (size_t) file_size + 1 );
    if ( !source )
    {
        fprintf( stderr, "Could not allocate memory for example '%s'.\n", path );
        fclose( file );
        exit( EXIT_FAILURE );
    }

    size_t bytes_read = fread( source, 1, (size_t) file_size, file );
    if ( bytes_read != (size_t) file_size )
    {
        fprintf( stderr, "Could not read example '%s': %s\n", path, strerror( errno ) );
        free( source );
        fclose( file );
        exit( EXIT_FAILURE );
    }

    source[bytes_read] = '\0';
    fclose( file );
    return source;
}

static void run_example( const char *name, const char *source )
{
    printf( "\n=== Program: %s ===\n", name );

    LexerMatchedTokenArray *tokens = lexer_lex( source );
    LexerMatchedTokenArray *filtered = lexer_filter_whitespace( tokens );
    AST *ast = parse_fun( filtered );

    AST_print_pretty_color( ast );
    run_fun_static_lazy( ast );

    lexer_matched_token_array_destroy( tokens );
    lexer_matched_token_array_destroy_no_strings( filtered );
}

static void run_examples( void )
{
    DIR *directory = opendir( EXAMPLES_DIRECTORY );
    if ( !directory )
    {
        fprintf( stderr, "Could not open '%s': %s\n", EXAMPLES_DIRECTORY, strerror( errno ) );
        exit( EXIT_FAILURE );
    }

    size_t example_count = 0;
    struct dirent *entry;
    while ( ( entry = readdir( directory ) ) != NULL )
    {
        if ( entry->d_name[0] == '.' || !has_example_extension( entry->d_name ) )
        {
            continue;
        }

        size_t path_length = strlen( EXAMPLES_DIRECTORY ) + 1 + strlen( entry->d_name ) + 1;
        char *path = malloc( path_length );
        if ( !path )
        {
            fprintf( stderr, "Could not allocate an example path.\n" );
            closedir( directory );
            exit( EXIT_FAILURE );
        }

        snprintf( path, path_length, "%s/%s", EXAMPLES_DIRECTORY, entry->d_name );
        char *source = read_example_file( path );

        run_example( entry->d_name, source );
        free( source );
        free( path );
        example_count++;
    }

    closedir( directory );

    if ( example_count == 0 )
    {
        fprintf( stderr, "No .fun files found in '%s'.\n", EXAMPLES_DIRECTORY );
    }
}

int main( void )
{
    /*
     * Some test programs (especially Church numeral recursion using Y combinator)
     * create deep call stacks.
     *
     * Raise the stack limit and use a large arena because deeply nested
     * Y-combinator examples allocate many environment frames.
     */
    arena_init( 32ULL * 1024 * 1024 * 1024 );
    lexer_init();

    struct rlimit rl;
    rl.rlim_cur = RLIM_INFINITY;
    rl.rlim_max = RLIM_INFINITY;
    setrlimit( RLIMIT_STACK, &rl );

    print_logo();
    puts( "Fun interpreter - static lazy evaluation" );
    puts( "Press Enter to run the bundled examples." );
    fflush( stdout );
    getchar();

    puts( "\nLazy evaluation examples" );
    run_examples();

    lexer_cleanup();
    arena_destroy();

    return 0;
}
