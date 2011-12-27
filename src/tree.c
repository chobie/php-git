/*
 * The MIT License
 *
 * Copyright (c) 2010 - 2011 Shuhei Tanuma
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "php_git.h"
#include <spl/spl_array.h>
#include <zend_interfaces.h>
#include <string.h>
#include <time.h>

PHPAPI zend_class_entry *git_tree_class_entry;

ZEND_BEGIN_ARG_INFO_EX(arginfo_git_tree__construct, 0, 0, 1)
    ZEND_ARG_INFO(0, repository)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_git_tree_add, 0, 0, 3)
    ZEND_ARG_INFO(0, hash)
    ZEND_ARG_INFO(0, name)
    ZEND_ARG_INFO(0, mode)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_git_tree_path, 0, 0, 1)
    ZEND_ARG_INFO(0, path)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_git_tree_get_entry, 0, 0, 1)
    ZEND_ARG_INFO(0, position)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_git_tree_get_entry_by_name, 0, 0, 1)
    ZEND_ARG_INFO(0, name)
ZEND_END_ARG_INFO()


ZEND_BEGIN_ARG_INFO_EX(arginfo_git_tree_remove, 0, 0, 1)
    ZEND_ARG_INFO(0, name)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_git_tree_resolve, 0, 0, 1)
    ZEND_ARG_INFO(0, name)
ZEND_END_ARG_INFO()


static void php_git_tree_free_storage(php_git_tree_t *obj TSRMLS_DC)
{
    zend_object_std_dtor(&obj->zo TSRMLS_CC);
    
    //Todo: which engine shoud free (php / libgit2)
    obj->object = NULL;
    efree(obj);
}

zend_object_value php_git_tree_new(zend_class_entry *ce TSRMLS_DC)
{
    zend_object_value retval;
    php_git_tree_t *obj;
    zval *tmp;

    obj = ecalloc(1, sizeof(*obj));
    zend_object_std_init( &obj->zo, ce TSRMLS_CC );
    zend_hash_copy(obj->zo.properties, &ce->default_properties, (copy_ctor_func_t) zval_add_ref, (void *) &tmp, sizeof(zval *));

    retval.handle = zend_objects_store_put(obj, 
        (zend_objects_store_dtor_t)zend_objects_destroy_object,
        (zend_objects_free_object_storage_t)php_git_tree_free_storage,
        NULL TSRMLS_CC);
    retval.handlers = zend_get_std_object_handlers();
    return retval;
}

PHP_METHOD(git_tree, count)
{
    php_git_tree_t *this= (php_git_tree_t *) zend_object_store_get_object(getThis() TSRMLS_CC);
    RETVAL_LONG(git_tree_entrycount(this->object));
}


PHP_METHOD(git_tree, path)
{
    php_git_tree_t *this= (php_git_tree_t *) zend_object_store_get_object(getThis() TSRMLS_CC);
    char *path;
    git_tree *tree;
    int ret, path_len= 0;
    git_tree_entry *entry;
    git_object *object;
    php_git_blob_t *blobobj;
    zval *git_tree, *entries, *git_object;
    git_oid *tree_oid;

    if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,
        "s", &path, &path_len) == FAILURE){
        return;
    }
    entry = git_tree_entry_byname(this->object,path);

    if(entry == NULL){
        return;
    }else{
        ret = git_tree_entry_2object(&object,git_object_owner(this->object), entry);

        if (ret != GIT_SUCCESS) {
            zend_throw_exception_ex(spl_ce_RuntimeException, 0 TSRMLS_CC,"something uncaught error happend.");
            RETURN_FALSE;
        }
        git_otype type = git_object_type(object);

        if(type == GIT_OBJ_TREE) {
            // Todo: refactoring below block
            tree = object;

            MAKE_STD_ZVAL(git_tree);
            MAKE_STD_ZVAL(entries);
            array_init(entries);
            object_init_ex(git_tree, git_tree_class_entry);
            php_git_tree_t *tobj = (php_git_tree_t *) zend_object_store_get_object(git_tree TSRMLS_CC);
            tobj->object = tree;
            int r = git_tree_entrycount(tree);
            int i = 0;
            for(i; i < r; i++){
                create_tree_entry_from_entry(&entry,git_tree_entry_byindex(tree,i),this->repository);
                add_next_index_zval(entries, entry);
            }
            add_property_zval(git_tree,"entries", entries);
            RETURN_ZVAL(git_tree,0,0);

        }else if(type == GIT_OBJ_BLOB){
            // Todo: refactoring below block
            MAKE_STD_ZVAL(git_object);
            object_init_ex(git_object, git_blob_class_entry);
            blobobj = (php_git_blob_t *) zend_object_store_get_object(git_object TSRMLS_CC);
            blobobj->object = (git_blob *)object;

            add_property_stringl_ex(git_object,"data", sizeof("data"), (char *)git_blob_rawcontent((git_blob *)object),git_blob_rawsize((git_blob *)object), 1 TSRMLS_CC);
            RETURN_ZVAL(git_object,0,0);
        } else {
            zend_throw_exception_ex(spl_ce_RuntimeException, 0 TSRMLS_CC,
                            "Git\\Tree::path can resolve GIT_OBJ_TREE or GIT_OBJ_BLOB. unhandled object type %d found.", git_object_type(object));
            RETURN_FALSE;
        }
    }
}

PHP_METHOD(git_tree, __construct)
{
    zval *z_repository;

    if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,
        "z", &z_repository) == FAILURE){
        return;
    }

    php_git_repository_t *git = (php_git_repository_t *) zend_object_store_get_object(z_repository TSRMLS_CC);
    php_git_tree_t *obj = (php_git_tree_t *) zend_object_store_get_object(getThis() TSRMLS_CC);
}

PHP_METHOD(git_tree, getIterator)
{
    php_git_tree_t *this = (php_git_tree_t *) zend_object_store_get_object(getThis() TSRMLS_CC);
    zval *iterator;
    
    MAKE_STD_ZVAL(iterator);
    object_init_ex(iterator,git_tree_iterator_class_entry);
    php_git_tree_iterator_t *obj = (php_git_tree_iterator_t *) zend_object_store_get_object(iterator TSRMLS_CC);
    obj->tree = this->object;
    obj->repository = this->repository;
    obj->offset = 0;
    RETURN_ZVAL(iterator,0,1);
}

PHP_METHOD(git_tree, getEntry)
{
    php_git_tree_t *this = (php_git_tree_t *) zend_object_store_get_object(getThis() TSRMLS_CC);
    git_tree_entry *entry;
    zval *git_tree_entry;
    int offset = 0;

    if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,
        "l", &offset) == FAILURE){
        return;
    }

    entry = git_tree_entry_byindex(this->object,offset);
    create_tree_entry_from_entry(&git_tree_entry, entry,this->repository);
    RETURN_ZVAL(git_tree_entry,0, 1);
}

PHP_METHOD(git_tree, getEntryByName)
{
    php_git_tree_t *this = (php_git_tree_t *) zend_object_store_get_object(getThis() TSRMLS_CC);
    git_tree_entry *entry;
    zval *git_tree_entry;
    char *name;
    long length = 0;

    if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,
    "s", &name, &length) == FAILURE){
        return;
    }

    entry = git_tree_entry_byname(this->object,name);
    create_tree_entry_from_entry(&git_tree_entry, entry,this->repository);
    RETURN_ZVAL(git_tree_entry,0, 1);
}

PHP_METHOD(git_tree, getEntries)
{
    php_git_tree_t *this = (php_git_tree_t *) zend_object_store_get_object(getThis() TSRMLS_CC);
    git_tree_entry *entry;
    zval *array_ptr,*entries;
    int i, r;
    
    r = git_tree_entrycount(this->object);

    MAKE_STD_ZVAL(entries);
    array_init(entries);
    for(i = 0; i < r; i++){
        create_tree_entry_from_entry(&array_ptr, git_tree_entry_byindex(this->object,i),this->repository);
        add_next_index_zval(entries,  array_ptr);
    }

    RETURN_ZVAL(entries,0,1);
}

// probably this method will be deplicated.
PHP_METHOD(git_tree, resolve)
{
    php_git_tree_t *this = (php_git_tree_t *) zend_object_store_get_object(getThis() TSRMLS_CC);
    git_tree_entry *entry;
    git_object *object;
    git_otype type;
    git_tree *tree;
    php_git_tree_t *tobj;
    int path_len, ret;
    char *path;
    zval *git_raw_object, *git_tree;
    
    if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,
        "s", &path, &path_len) == FAILURE){
        return;
    }

    ret = git_tree_entry_resolve_byname(&entry, this->object, this->repository, path);
    if(ret != GIT_SUCCESS) {
        RETURN_FALSE;
    }

    ret = git_tree_entry_2object(&object, this->repository, entry);
    if(ret != GIT_SUCCESS) {
        RETURN_FALSE;
    }

    type = git_object_type(object);
    if (type == GIT_OBJ_BLOB) {

        MAKE_STD_ZVAL(git_raw_object);
        object_init_ex(git_raw_object, git_blob_class_entry);
        php_git_blob_t *blobobj = (php_git_blob_t *) zend_object_store_get_object(git_raw_object TSRMLS_CC);
        blobobj->object = (git_blob *)object;

        add_property_stringl_ex(git_raw_object,"data", sizeof("data"), (char *)git_blob_rawcontent((git_blob *)object),git_blob_rawsize((git_blob *)object), 1 TSRMLS_CC);
        RETURN_ZVAL(git_raw_object,0,1);
    } else if(type == GIT_OBJ_TREE) {
        tree = object;

        MAKE_STD_ZVAL(git_tree);
        object_init_ex(git_tree, git_tree_class_entry);

        tobj = (php_git_tree_t *) zend_object_store_get_object(git_tree TSRMLS_CC);
        tobj->object = tree;
        tobj->repository = this->repository;

        RETURN_ZVAL(git_tree,0,1);

    } else{
        zend_throw_exception_ex(spl_ce_RuntimeException, 0 TSRMLS_CC,
            "Git\\Tree\\Entry::toObject can convert GIT_OBJ_TREE or GIT_OBJ_BLOB. unhandled object type %d found.", git_object_type(object));
        RETURN_FALSE;
    }

}

static zend_function_entry php_git_tree_methods[] = {
    PHP_ME(git_tree, resolve,     NULL, ZEND_ACC_PUBLIC)
    PHP_ME(git_tree, __construct, arginfo_git_tree__construct, ZEND_ACC_PUBLIC)
    PHP_ME(git_tree, getEntry,    arginfo_git_tree_get_entry,  ZEND_ACC_PUBLIC)
    PHP_ME(git_tree, getEntries,  NULL,                        ZEND_ACC_PUBLIC)
    PHP_ME(git_tree, getIterator, NULL,                        ZEND_ACC_PUBLIC)
    PHP_ME(git_tree, count,       NULL,                        ZEND_ACC_PUBLIC)
    PHP_ME(git_tree, path,        arginfo_git_tree_path,       ZEND_ACC_PUBLIC)
    PHP_ME(git_tree, getEntryByName, arginfo_git_tree_get_entry_by_name, ZEND_ACC_PUBLIC)
    {NULL, NULL, NULL}
};

void git_init_tree(TSRMLS_D)
{
    zend_class_entry ce;
    INIT_NS_CLASS_ENTRY(ce, PHP_GIT_NS,"Tree", php_git_tree_methods);
    git_tree_class_entry = zend_register_internal_class_ex(&ce, git_object_class_entry, NULL TSRMLS_CC);
    git_tree_class_entry->create_object = php_git_tree_new;
    
    zend_class_implements(git_tree_class_entry TSRMLS_CC, 2, spl_ce_Countable,zend_ce_aggregate);
}
